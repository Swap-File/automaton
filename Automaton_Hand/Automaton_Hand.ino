#include <ArduinoBLE.h>
#include "LSM6DS3.h"
#include "Wire.h"
#include <mic.h>
#include "Fusion.h"
#include <arduinoFFT.h>
#include <Adafruit_NeoPixel.h>

//add autohand detection via pins
//add battery level low blue blinking

Adafruit_NeoPixel pixels2(22, D2, NEO_GRB + NEO_KHZ800);

bool hand = false; //true = left & red  false = right and green

FusionAhrs ahrs;
#define IMU_SAMPLE_RATE (0.01f)  //lie about sample rate for nice filtering


#define int1Pin PIN_LSM6DS3TR_C_INT1

int tap_event_count = 0;
int tap_event_last = 0;
volatile static uint32_t last_tap_time = 0;
volatile uint8_t interruptCount = 0;


int fps = 0;

arduinoFFT FFT = arduinoFFT();
#define FFT_SAMPLES 16
double fft_vReal[FFT_SAMPLES];
double fft_vImag[FFT_SAMPLES];
uint8_t fft_scaled_255[FFT_SAMPLES / 2] =  {0};
volatile bool mic_buffer_full = false;

// MICROPHONE

mic_config_t mic_config{  // 16000/(1600/2) = 20 fps
  .channel_cnt = 1,
  .sampling_rate = 16000,
  .buf_size = 1600,
  .debug_pin = 0
};

NRF52840_ADC_Class Mic(&mic_config);

static void audio_rec_callback(uint16_t *buf, uint32_t buf_len) {

  if (mic_buffer_full) return;  //should never happen, here for safety

  for (uint32_t i = 0 ; i < FFT_SAMPLES; i++) {
    fft_vReal[i] = (int16_t)buf[buf_len - FFT_SAMPLES + i];  // grab the last 16 samples
    fft_vImag[i] = 0;
  }
  mic_buffer_full = true;
}


// IMU
FusionEuler euler;
uint8_t stepCount = 0;
LSM6DS3 myIMU(I2C_MODE, 0x6A);

void int1ISR()
{
  uint32_t tap_time = millis();
  if (tap_time - last_tap_time < 100) { // too soon
    return;
  }
  interruptCount++;
  last_tap_time = tap_time;

}


void setup() {
  Serial.begin(9600);
  
  battery_init();
  
  pixels2.begin();

  FusionAhrsInitialise(&ahrs);

  // Set AHRS algorithm settings
  const FusionAhrsSettings settings = {
    .gain = 0.5f,
    .accelerationRejection = 10.0f,
    .magneticRejection = 0.0f,
    .rejectionTimeout = 5.0f,
  };
  FusionAhrsSetSettings(&ahrs, &settings);


  if (myIMU.begin() != 0) {
    Serial.println("myIMU error");
  } else {
    Serial.println("myIMU OK!");
  }

  pinMode(int1Pin, INPUT);
  attachInterrupt(digitalPinToInterrupt(int1Pin), int1ISR, RISING);

  myIMU.writeRegister(LSM6DS3_ACC_GYRO_TAP_CFG1, 0x8E);// INTERRUPTS_ENABLE, SLOPE_FDS
  myIMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL10_C, 0x3C);  //enable pedometer
  myIMU.writeRegister(LSM6DS3_ACC_GYRO_TAP_THS_6D, 0x8C);  //tap threshold
  //myIMU.writeRegister(LSM6DS3_ACC_GYRO_INT_DUR2, 0x7F); //tap duraction
  myIMU.writeRegister(LSM6DS3_ACC_GYRO_MD1_CFG, 0b01000000); //single tap route to int

  Mic.set_callback(audio_rec_callback);
  if (!Mic.begin()) {
    Serial.println("Mic initialization failed");
    while (1);
  }

  Serial.println("Mic initialization done.");

  // configure the button pin as input
  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);
  digitalWrite(LEDR, HIGH);
  digitalWrite(LEDG, HIGH);
  digitalWrite(LEDB, HIGH);

  pinMode(P0_14, OUTPUT);
  digitalWrite(P0_14, LOW);

  BLE.begin();

  // start scanning for peripherals
  BLE.scanForUuid("19b10000-e8f2-537e-4f6c-d104768a1214");

  if (hand) {
    digitalWrite(LEDR, LOW);
  } else {
    digitalWrite(LEDG, LOW);
  }
}


inline bool update_mic(void) {  //this takes ~1.5ms

  if (mic_buffer_full) {

    FFT.Windowing(fft_vReal, FFT_SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.Compute(fft_vReal, fft_vImag, FFT_SAMPLES, FFT_FORWARD);
    FFT.ComplexToMagnitude(fft_vReal, fft_vImag, FFT_SAMPLES);


    static int fft_slow_avg_min[FFT_SAMPLES / 2] =  {0};
    static int fft_fast_avg_max[FFT_SAMPLES / 2] =  {0};

    const int DYNAMIC_RANGE = 64;  //increase if too sensative, decrease if not sensative enough

    for (int i = 0; i < FFT_SAMPLES / 2; i++) {
      int fft_new_sample = fft_vReal[i];
      // de-emphasize low bands so the dynamic range across the entire spectrum is more constant
      if (i == 0)
        fft_new_sample = fft_new_sample * .25;
      if (i == 1)
        fft_new_sample = fft_new_sample * .5;
      if (i == 2)
        fft_new_sample = fft_new_sample * .75;
      fft_slow_avg_min[i] = fft_new_sample * 0.05 + 0.95 * fft_slow_avg_min[i];
      fft_new_sample = max(fft_new_sample - (fft_slow_avg_min[i] + DYNAMIC_RANGE), 0);
      fft_fast_avg_max[i] = max(max(fft_fast_avg_max[i] * .97, fft_new_sample), fft_slow_avg_min[i] );
      fft_scaled_255[i] = constrain(map(fft_new_sample, 0, fft_fast_avg_max[i], 0, 255), 0, 255);
    }

    mic_buffer_full = false;
    return true;
  }
  return false;
}

inline void update_imu(void) {

  const FusionVector gyroscope = {myIMU.readFloatGyroX(), myIMU.readFloatGyroY(), myIMU.readFloatGyroZ()}; // replace this with actual gyroscope data in degrees/s
  const FusionVector accelerometer = {myIMU.readFloatAccelX(), myIMU.readFloatAccelY(), myIMU.readFloatAccelZ()}; // replace this with actual accelerometer data in g
  FusionAhrsUpdateNoMagnetometer(&ahrs, gyroscope, accelerometer, IMU_SAMPLE_RATE);
  euler = FusionQuaternionToEuler(FusionAhrsGetQuaternion(&ahrs));

  myIMU.readRegister(&stepCount, LSM6DS3_ACC_GYRO_STEP_COUNTER_L);

}

bool update_mic_and_imu(void) {
  
  monitor_battery_level();
  
  if (interruptCount > 0 &&  millis() - last_tap_time > 500) {
    tap_event_count++;
    tap_event_last = interruptCount;
    interruptCount = 0;
  }


  if (update_mic()) { // updates at 20hz set by sample rate and buffer size
    update_imu(); //update the IMU at the same cadence as the microphone
    fps++;
    return true;
  }
  return false;
}


void debug_led_show(void) {

  pixels2.clear();
  for (int i = 0; i < FFT_SAMPLES / 2; i++) {
    pixels2.setPixelColor( i, pixels2.Color(0, fft_scaled_255[i], 0)  );
    pixels2.setPixelColor(FFT_SAMPLES - 1 - i, pixels2.Color(0, fft_scaled_255[i], 0)  );

  }
  pixels2.show();

}



void controlLed(BLEDevice peripheral) {
  // connect to the peripheral
  Serial.println("Connecting ...");

  if (peripheral.connect()) {
    Serial.println("Connected");
  } else {
    Serial.println("Failed to connect!");
    return;
  }

  // discover peripheral attributes
  Serial.println("Discovering attributes ...");
  if (peripheral.discoverAttributes()) {
    Serial.println("Attributes discovered");
  } else {
    Serial.println("Attribute discovery failed!");
    peripheral.disconnect();
    return;
  }

  // retrieve the LED characteristic
  BLECharacteristic ledCharacteristic;
  if (hand)
    ledCharacteristic = peripheral.characteristic("19b10001-e8f2-537e-4f6c-d104768a1215");
  else
    ledCharacteristic = peripheral.characteristic("19b10001-e8f2-537e-4f6c-d104768a1214");



  if (!ledCharacteristic) {
    Serial.println("Peripheral does not have LED characteristic!");
    peripheral.disconnect();
    return;
  } else if (!ledCharacteristic.canWrite()) {
    Serial.println("Peripheral does not have a writable LED characteristic!");
    peripheral.disconnect();
    return;
  }

  while (peripheral.connected()) {
    static int fps = 0;
    static int total_time = 0;
    uint32_t start_it = micros();
    if (update_mic_and_imu()) {

      static uint32_t last_time = 0;

      static uint8_t payload[15] = {0};

      //other stuff to send:
      //add yaw pitch roll mapped to 8 bit
      //add step 8 bit
      //add event count
      //add event itself

      Serial.print(euler.angle.roll);
      Serial.print('\t');
      Serial.print(euler.angle.pitch);
      Serial.print('\t');
      Serial.print(euler.angle.yaw);
      Serial.print('\t');
      Serial.print("\tStep: ");
      Serial.println(stepCount);


      payload[14] = tap_event_last;
      payload[13] = tap_event_count;
      payload[12] = stepCount;
      payload[11] = constrain(map(euler.angle.yaw + 180, 0, 360, 0, 256), 0, 255);
      payload[10] = constrain(map(euler.angle.pitch + 180, 0, 360, 0, 256), 0, 255);
      payload[9] = constrain(map(euler.angle.roll + 180, 0, 360, 0, 256), 0, 255);
      payload[8] = fft_scaled_255[7];
      payload[7] = fft_scaled_255[6];
      payload[6] = fft_scaled_255[5];
      payload[5] = fft_scaled_255[4];
      payload[4] = fft_scaled_255[3];
      payload[3] = fft_scaled_255[2];
      payload[2] = fft_scaled_255[1];
      payload[1] = fft_scaled_255[0];
      payload[0]++;
      ledCharacteristic.writeValue(payload, 15);

      if (hand) {
        digitalWrite(LEDR, payload[0] & 0x01);
      } else {
        digitalWrite(LEDG, payload[0] & 0x01);
      }



             
      debug_led_show();
      fps++;
      total_time += micros() - start_it;

    }

    static uint32_t last_time = 0;
    if (millis() - last_time > 1000) {
      last_time += 1000;
      if ( millis() - last_time > 1000)
        last_time = millis();
      Serial.print("\t");
      Serial.print( total_time / 10000);
      Serial.print("% Load \tFPS:");
      Serial.println(fps);
      fps = 0;
      total_time = 0;
    }

  }
  Serial.println("Peripheral disconnected");
  if (hand) {
    digitalWrite(LEDR, LOW);
  } else {
    digitalWrite(LEDG, LOW);
  }
}

void loop() {

  // debug IMU & MIC, 16 LEDs on Pin 2 for testing
  /*
    while (1) {
    uint32_t start_it = micros();
    if (update_mic_and_imu()) {
      debug_led_print();
      Serial.print(micros() - start_it);
      Serial.print('\t');
      Serial.print(euler.angle.roll);
      Serial.print('\t');
      Serial.print(euler.angle.pitch);
      Serial.print('\t');
      Serial.print(euler.angle.yaw);
      Serial.print('\t');
      Serial.print("\tStep: ");
      Serial.println(stepCount);
    }
    }
  */



  update_mic_and_imu();  // keep data warm while not connected

  // check if a peripheral has been discovered
  BLEDevice peripheral = BLE.available();

  if (peripheral) {
    // discovered a peripheral, print out address, local name, and advertised service
    Serial.print("Found ");
    Serial.print(peripheral.address());
    Serial.print(" '");
    Serial.print(peripheral.localName());
    Serial.print("' ");
    Serial.print(peripheral.advertisedServiceUuid());
    Serial.println();

    if (peripheral.localName() != "AUTOMATON") {
      return;
    }

    // stop scanning
    BLE.stopScan();
    controlLed(peripheral);

    // peripheral disconnected, start scanning again
    BLE.scanForUuid("19b10000-e8f2-537e-4f6c-d104768a1214");

  }
}

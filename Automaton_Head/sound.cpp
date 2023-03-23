#include <arduinoFFT.h>
#include <PDM.h>
#include "sound.h"

volatile bool pdm_processed = true;

#define PDM_SAMPLES 1600  //20hz at 16,000 sample rate

arduinoFFT FFT = arduinoFFT();
double fft_vReal[FFT_SAMPLES * 2];
double fft_vImag[FFT_SAMPLES * 2];

static void onPDMdata(void) {

  int16_t sampleBuffer[PDM_SAMPLES / 2 ];
  // query the number of bytes available
  int bytesAvailable = PDM.available();

  // read into the sample buffer
  PDM.read(sampleBuffer, bytesAvailable);

  for (uint32_t i = 0; i < FFT_SAMPLES* 2; i++) {
    fft_vReal[i] = (int16_t)sampleBuffer[PDM_SAMPLES / 2 - FFT_SAMPLES * 2+ i]; // grab the last 16 samples
    fft_vImag[i] = 0;
  }
  
  pdm_processed = false;
}


bool sound_update(struct cpu_struct *hand_data) {  //this takes ~1.5ms

  if (pdm_processed == false) {

    FFT.Windowing(fft_vReal, FFT_SAMPLES* 2, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.Compute(fft_vReal, fft_vImag, FFT_SAMPLES* 2, FFT_FORWARD);
    FFT.ComplexToMagnitude(fft_vReal, fft_vImag, FFT_SAMPLES* 2);


    static int fft_slow_avg_min[FFT_SAMPLES ] = { 0 };
    static int fft_fast_avg_max[FFT_SAMPLES ] = { 0 };

    const int DYNAMIC_RANGE = 64;  //increase if too sensative, decrease if not sensative enough

    for (int i = 0; i < FFT_SAMPLES; i++) {
      int fft_new_sample = fft_vReal[i];
      // de-emphasize low bands so the dynamic range across the entire spectrum is more constant
      if (i == 0)
        fft_new_sample = fft_new_sample * .125;
      if (i == 1)
        fft_new_sample = fft_new_sample * .25;
      if (i == 2)
        fft_new_sample = fft_new_sample * .5;
      fft_slow_avg_min[i] = fft_new_sample * 0.05 + 0.95 * fft_slow_avg_min[i];
      fft_new_sample = max(fft_new_sample - (fft_slow_avg_min[i] + DYNAMIC_RANGE), 0);
      fft_fast_avg_max[i] = max(max(fft_fast_avg_max[i] * .97, fft_new_sample), fft_slow_avg_min[i]);
      hand_data->fft[i] = constrain(map(fft_new_sample, 0, fft_fast_avg_max[i], 0, 255), 0, 255);
    }

    pdm_processed = true;
    return true;
  }
  return false;
}


void sound_init(void) {
  PDM.setBufferSize(PDM_SAMPLES);
  // configure the data receive callback
  PDM.onReceive(onPDMdata);

  // optionally set the gain, defaults to 20
  // PDM.setGain(30);

  // initialize PDM with:
  // - one channel (mono mode)
  // - a 16 kHz sample rate
  if (!PDM.begin(1, 16000)) {
    Serial.println("Failed to start PDM!");
    while (1) yield();
  }
}

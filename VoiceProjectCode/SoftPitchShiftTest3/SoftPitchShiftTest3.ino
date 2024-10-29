#include "AudioTools.h"

// Parameters
int pitch_buffer_size = 256;
float pitch_shift = 0.75;
AudioInfo info(32000, 2, 32);  // 24 kHz, stereo, 32-bit audio

// Initialize I2S microphone input and DAC output
I2SStream i2s_mic;
I2SStream i2s_out;

// Filter, Equalizer, and Pitch Shift
FilteredStream<int32_t, float> filtered(i2s_mic, info.channels); // Apply FIR filter on input
Equilizer3Bands eq(filtered);  // Apply equalizer after FIR filter
PitchShiftOutput<int32_t, VariableSpeedRingBuffer<int32_t>> pitchShift(i2s_out); // Apply pitch shift on equalized audio

StreamCopy copier(pitchShift, eq); // Copies filtered, equalized, and pitch-shifted audio from mic to DAC

// FIR filter coefficients for a low-pass filter
float coef[] = { 0.021, 0.096, 0.146, 0.096, 0.021 };

// Equalizer configuration
ConfigEquilizer3Bands cfg_eq;

// Arduino setup
void setup(void) {
  // Start serial communication for debugging
  Serial.begin(115200);
  AudioLogger::instance().begin(Serial, AudioLogger::Warning);

  // Configure I2S for microphone input
  auto config_mic = i2s_mic.defaultConfig(RX_MODE);   // Set I2S to receive mode
  config_mic.copyFrom(info);
  config_mic.i2s_format = I2S_STD_FORMAT;
  config_mic.is_master = true;
  config_mic.port_no = 0;  // Set number of channels (1 for mono)
  config_mic.pin_bck = 32;  // BCK pin (set according to your ESP32 setup)
  config_mic.pin_ws = 25;   // WS pin (set according to your ESP32 setup)
  config_mic.pin_data = 33; // Data in pin from mic (set according to your ESP32 setup)
  i2s_mic.begin(config_mic); // Initialize I2S mic

  // Configure I2S for DAC output
  auto config_out = i2s_out.defaultConfig(TX_MODE);  // Set I2S to transmission mode
  config_out.copyFrom(info);
  config_out.i2s_format = I2S_STD_FORMAT;
  config_out.is_master = true;
  config_out.port_no = 1;  // Set number of channels (1 for mono)
  config_out.pin_bck = 14;  // BCK pin (shared with mic)
  config_out.pin_ws = 15;   // WS pin (shared with mic)
  config_out.pin_data = 22; // Data out pin to DAC (set according to your ESP32 setup)
  i2s_out.begin(config_out); // Initialize I2S for DAC output

  // Configure FIR filters for all channels
  filtered.setFilter(0, new FIR<float>(coef));  // Set FIR filter for the first channel
  filtered.setFilter(1, new FIR<float>(coef));  // Set FIR filter for the second channel

  // Configure equalizer
  cfg_eq = eq.defaultConfig();
  cfg_eq.setAudioInfo(info); // Use channels, bits_per_sample, and sample_rate from info
  cfg_eq.gain_low = 0.3;     // Lower the bass slightly to reduce muddiness
  cfg_eq.gain_medium = 0.8;  // Boost mid frequencies for vocal clarity
  cfg_eq.gain_high = 1.2;    // Boost high frequencies to enhance articulation and detail
  eq.begin(cfg_eq);

  // Configure pitch shifter
  auto pcfg = pitchShift.defaultConfig();
  pcfg.copyFrom(info);          // Use same audio info for pitch shifter
  pcfg.pitch_shift = pitch_shift; // Set pitch shift factor
  pcfg.buffer_size = pitch_buffer_size;  // Set buffer size
  pitchShift.begin(pcfg);

  Serial.println("I2S microphone, filter, equalizer, pitch shift, and DAC started...");
}

// Arduino loop - continuously copy audio from mic to DAC with filtering, equalizing, and pitch shifting
void loop() {
  copier.copy();  // Copy filtered, equalized, and pitch-shifted audio
}

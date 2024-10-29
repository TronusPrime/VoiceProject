#include "AudioTools.h"

int pitch_buffer_size = 256;
float pitch_shift = 0.80;
AudioInfo info(24000, 2, 32);  // 44.1 kHz, mono, 16-bit audio

// Initialize I2S microphone input and DAC output
I2SStream i2s_mic;
I2SStream i2s_out;
PitchShiftOutput<int32_t, VariableSpeedRingBuffer<int32_t>> pitchShift(i2s_out);
StreamCopy copier(pitchShift, i2s_mic); // Copies audio from mic to DAC (pitch-shifted)

void setup(void) {
  // Start serial communication for debugging
  Serial.begin(115200);
  AudioLogger::instance().begin(Serial, AudioLogger::Warning);

  // Configure I2S for microphone input
  auto config_mic = i2s_mic.defaultConfig(RX_MODE);   // Set I2S to receive mode
  config_mic.copyFrom(info); 
  config_mic.i2s_format = I2S_STD_FORMAT;
  config_mic.is_master = true;
  config_mic.port_no = 0;// Set number of channels (1 for mono)
  config_mic.pin_bck = 32;                            // BCK pin (set according to your ESP32 setup)
  config_mic.pin_ws = 25;                             // WS pin (set according to your ESP32 setup)
  config_mic.pin_data = 33;                           // Data in pin from mic (set according to your ESP32 setup)
  i2s_mic.begin(config_mic);                          // Initialize I2S mic

  // Configure I2S for DAC output
  auto config_out = i2s_out.defaultConfig(TX_MODE);   // Set I2S to transmission mode
  config_out.copyFrom(info); 
  config_out.i2s_format = I2S_STD_FORMAT;
  config_out.is_master = true;
  config_out.port_no = 1;// Set number of channels (1 for mono)
  config_out.pin_bck = 14;                            // BCK pin (shared with mic)
  config_out.pin_ws = 15;                             // WS pin (shared with mic)
  config_out.pin_data = 22;                           // Data out pin to DAC (set according to your ESP32 setup)
  i2s_out.begin(config_out);                          // Initialize I2S for DAC output

  // Configure pitch shifter
  auto pcfg = pitchShift.defaultConfig();
  pcfg.copyFrom(info);                                // Use same audio info for pitch shifter
  pcfg.pitch_shift = pitch_shift;                     // Set pitch shift factor (1.5x)
  pcfg.buffer_size = pitch_buffer_size;               // Set buffer size
  pitchShift.begin(pcfg);

  Serial.println("I2S microphone and DAC started...");
}

void loop() {
  copier.copy();                                      // Copy audio from mic to DAC (pitch-shifted)
}

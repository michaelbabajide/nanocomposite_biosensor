#include <Wire.h>
#include "AD5933.h"

// Frequency sweep parameters
#define START_FREQ   (1000)  // 1 kHz
#define END_FREQ    (100000) // 100 kHz
#define FREQ_INCR    (2000)  // 2 kHz increments
#define NUM_INCR    ((END_FREQ - START_FREQ) / FREQ_INCR)
#define REF_RESIST   (470000)  // 470k ohm reference resistor - VERIFIED THIS IS CORRECT

// Single gain factor for calibration
double gain_factor = 0.0;

// Output voltage range settings
#define OUTPUT_VOLTAGE_RANGE CTRL_OUTPUT_RANGE_3 // 0.4V p-p for better SNR with 470k

void setup(void) {
 Wire.begin();
 Serial.begin(115200);

 while (!Serial) {
  delay(10);
 }

 Serial.println(F("AD5933 Enhanced Impedance Analyzer"));
 Serial.println(F("==================================="));

 if (!AD5933::reset()) {
  Serial.println(F("ERROR: Failed to reset AD5933!"));
  while (true);
 }
 Serial.println(F("✓ AD5933 reset successful"));

 if (!configureAD5933()) {
  Serial.println(F("ERROR: Failed to configure AD5933!"));
  while (true);
 }
 Serial.println(F("✓ AD5933 configured successfully"));

 Serial.println(F("\nStarting calibration..."));
 Serial.println(F("ENSURE 470kΩ REFERENCE RESISTOR IS CONNECTED!"));
 delay(2000); // Give user time to read

 if (performCalibration()) {
  Serial.println(F("✓ Calibration completed!"));
  Serial.print(F("Final Average Gain Factor: "));
  Serial.println(gain_factor, 12);
 } else {
  Serial.println(F("ERROR: Calibration failed!"));
  while (true);
 }

 Serial.println(F("\n==================================="));
 Serial.println(F("Setup complete!"));
 Serial.println(F("Connect test impedance and send any character"));
 Serial.println(F("===================================\n"));
}

void loop(void) {
 if (Serial.available() > 0) {
  while (Serial.available() > 0) {
   Serial.read();
  }

  performImpedanceMeasurement();
  Serial.println(F("\nSend any character for another measurement."));
 }

 delay(100);
}

bool configureAD5933() {
 if (!AD5933::setInternalClock(true)) return false;

 AD5933 instance;
 if (!instance.setRange(OUTPUT_VOLTAGE_RANGE)) return false;
 if (!AD5933::setPGAGain(PGA_GAIN_X1)) return false;
 if (!AD5933::setStartFrequency(START_FREQ)) return false;
 if (!AD5933::setIncrementFrequency(FREQ_INCR)) return false;
 if (!AD5933::setNumberIncrements(NUM_INCR)) return false;

 // INCREASED SETTLING CYCLES FOR MORE STABLE MEASUREMENTS
 if (!instance.setSettlingCycles(50)) {
  Serial.println(F("Warning: Using default settling cycles"));
 }

 return true;
}

bool performCalibration() {
 Serial.println(F("Calibrating with 470kΩ reference resistor..."));

 // Initialize frequency sweep for calibration
 if (!(AD5933::setPowerMode(POWER_STANDBY) &&
    AD5933::setControlMode(CTRL_INIT_START_FREQ) &&
    AD5933::setControlMode(CTRL_START_FREQ_SWEEP))) {
  return false;
 }

 delay(100); // WAIT FOR INITIAL SETTLING

 int point_count = 0;
 unsigned long current_freq = START_FREQ;
 double total_gain = 0.0;
 int valid_points = 0;

 // Collect calibration data point by point
 while ((AD5933::readStatusRegister() & STATUS_SWEEP_DONE) != STATUS_SWEEP_DONE && point_count <= NUM_INCR) {

  // WAIT FOR DATA READY
  delay(50);

  int real, imag;

  if (!AD5933::getComplexData(&real, &imag)) {
   Serial.print(F("Failed to get data at "));
   Serial.println(current_freq);
  } else {
   double magnitude = sqrt((double)real * real + (double)imag * imag);

   Serial.print(F("Cal point: "));
   Serial.print(current_freq);
   Serial.print(F(" Hz, Mag: "));
   Serial.print(magnitude);
   Serial.print(F(", Real: "));
   Serial.print(real);
   Serial.print(F(", Imag: "));
   Serial.println(imag);

   // Check for valid magnitude to avoid division by zero
   if (magnitude > 1.0) {
    double current_gain = 1.0 / (REF_RESIST * magnitude);
    total_gain += current_gain;
    valid_points++;

    Serial.print(F(" -> Gain factor: "));
    Serial.println(current_gain, 12);
   } else {
    Serial.print(F("Warning: Low magnitude at "));
    Serial.print(current_freq);
    Serial.print(F(" Hz: "));
    Serial.println(magnitude);
   }
  }

  point_count++;
  current_freq += FREQ_INCR;

  // PREVENT OVERFLOW
  if (point_count > NUM_INCR) break;

  AD5933::setControlMode(CTRL_INCREMENT_FREQ);
  delay(10); // Small delay between increments
 }

 AD5933::setPowerMode(POWER_STANDBY);

 Serial.print(F("Total points measured: "));
 Serial.println(point_count);
 Serial.print(F("Valid points for averaging: "));
 Serial.println(valid_points);

 // Calculate the average gain factor
 if (valid_points > 0) {
  gain_factor = total_gain / valid_points;
  return true;
 }

 return false;
}
// The getGainFactor function is no longer needed since we have a single gain factor.
// It is replaced with a direct use of the global gain_factor variable.
void performImpedanceMeasurement() {
 Serial.println(F("\nStarting measurement..."));
 Serial.println(F("Frequency (Hz),Real Z (ohms),Imaginary Z (ohms),|Z| (ohms),Phase (degrees)"));

 // Initialize frequency sweep
 if (!(AD5933::setPowerMode(POWER_STANDBY) &&
    AD5933::setControlMode(CTRL_INIT_START_FREQ) &&
    AD5933::setControlMode(CTRL_START_FREQ_SWEEP))) {
  Serial.println(F("ERROR: Failed to initialize sweep"));
  return;
 }

 delay(100); // Initial settling
 unsigned long current_freq = START_FREQ;
 int point_count = 0;

 // Collect and process data point by point
 while ((AD5933::readStatusRegister() & STATUS_SWEEP_DONE) != STATUS_SWEEP_DONE && point_count <= NUM_INCR) {

  delay(20); // Wait for data ready

  int real, imag;

  if (!AD5933::getComplexData(&real, &imag)) {
   Serial.print(current_freq);
   Serial.println(F(",ERROR,ERROR,ERROR,ERROR"));
   current_freq += FREQ_INCR;
   point_count++;
   if (point_count <= NUM_INCR) {
    AD5933::setControlMode(CTRL_INCREMENT_FREQ);
   }
   continue;
  }

  // Calculate magnitude and phase of DFT result
  double magnitude = sqrt((double)real * real + (double)imag * imag);
  double phase_rad = atan2(imag, real);
  double phase_deg = phase_rad * 180.0 / PI;

  // Apply calibration to get impedance
  double impedance_magnitude = 0;

  if (gain_factor > 0 && magnitude > 0) {
   // CORRECTED impedance calculation per AD5933 datasheet
   impedance_magnitude = 1.0 / (gain_factor * magnitude);
  } else {
   Serial.print(current_freq);
   Serial.println(F(",ERROR,ERROR,ERROR,ERROR"));
   current_freq += FREQ_INCR;
   point_count++;
   if (point_count <= NUM_INCR) {
    AD5933::setControlMode(CTRL_INCREMENT_FREQ);
   }
   continue;
  }

  // Calculate real and imaginary components of impedance
  double real_impedance = impedance_magnitude * cos(phase_rad);
  double imag_impedance = impedance_magnitude * sin(phase_rad);

  // VALIDATE RESULTS - flag suspicious values
  bool suspicious = false;
  if (real_impedance < 0 && abs(phase_deg) < 85) { // Real impedance shouldn't be negative for resistive samples
   suspicious = true;
  }
  if (impedance_magnitude < 10 || impedance_magnitude > 1e8) { // Unreasonable magnitudes
   suspicious = true;
  }

  // Output CSV data
  Serial.print(current_freq);
  Serial.print(F(","));
  Serial.print(real_impedance, 6);
  if (suspicious) Serial.print(F("*")); // Mark suspicious data
  Serial.print(F(","));
  Serial.print(imag_impedance, 6);
  Serial.print(F(","));
  Serial.print(impedance_magnitude, 6);
  Serial.print(F(","));
  Serial.println(phase_deg, 3);

  current_freq += FREQ_INCR;
  point_count++;

  if (point_count <= NUM_INCR) {
   AD5933::setControlMode(CTRL_INCREMENT_FREQ);
  }
 }

 AD5933::setPowerMode(POWER_STANDBY);

 Serial.println(F("\n=== Measurement Summary ==="));
 Serial.print(F("Frequency range: "));
 Serial.print(START_FREQ);
 Serial.print(F(" Hz to "));
 Serial.print(END_FREQ);
 Serial.println(F(" Hz"));
 Serial.print(F("Data points: "));
 Serial.println(point_count);
 Serial.print(F("Reference resistor: "));
 Serial.print(REF_RESIST);
 Serial.println(F(" ohms"));
 Serial.println(F("Note: Suspicious values marked with *"));
 Serial.println(F("==========================="));
}
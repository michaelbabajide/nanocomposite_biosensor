// #include <Wire.h>
// #include "AD5933.h"

// // Frequency sweep parameters
// #define START_FREQ   (1000)  // 1 kHz
// #define END_FREQ    (100000) // 100 kHz
// #define FREQ_INCR    (2000)  // 2 kHz increments
// #define NUM_INCR    ((END_FREQ - START_FREQ) / FREQ_INCR)
// #define REF_RESIST   (12000)  // 470k ohm reference resistor - VERIFIED THIS IS CORRECT

// // Single gain factor for calibration
// double gain_factor = 0.0;

// // Output voltage range settings
// #define OUTPUT_VOLTAGE_RANGE CTRL_OUTPUT_RANGE_3 // 0.4V p-p for better SNR with 470k

// void setup(void) {
//  Wire.begin();
//  Serial.begin(115200);

//  while (!Serial) {
//   delay(10);
//  }

//  Serial.println(F("AD5933 Enhanced Impedance Analyzer"));
//  Serial.println(F("==================================="));

//  if (!AD5933::reset()) {
//   Serial.println(F("ERROR: Failed to reset AD5933!"));
//   while (true);
//  }
//  Serial.println(F("✓ AD5933 reset successful"));

//  if (!configureAD5933()) {
//   Serial.println(F("ERROR: Failed to configure AD5933!"));
//   while (true);
//  }
//  Serial.println(F("✓ AD5933 configured successfully"));

//  Serial.println(F("\nStarting calibration..."));
//  Serial.println(F("ENSURE 470kΩ REFERENCE RESISTOR IS CONNECTED!"));
//  delay(2000); // Give user time to read

//  if (performCalibration()) {
//   Serial.println(F("✓ Calibration completed!"));
//   Serial.print(F("Final Average Gain Factor: "));
//   Serial.println(gain_factor, 12);
//  } else {
//   Serial.println(F("ERROR: Calibration failed!"));
//   while (true);
//  }

//  Serial.println(F("\n==================================="));
//  Serial.println(F("Setup complete!"));
//  Serial.println(F("Connect test impedance and send any character"));
//  Serial.println(F("===================================\n"));
// }

// void loop(void) {
//  if (Serial.available() > 0) {
//   while (Serial.available() > 0) {
//    Serial.read();
//   }

//   performImpedanceMeasurement();
//   Serial.println(F("\nSend any character for another measurement."));
//  }

//  delay(100);
// }

// bool configureAD5933() {
//  if (!AD5933::setInternalClock(true)) return false;

//  AD5933 instance;
//  if (!instance.setRange(OUTPUT_VOLTAGE_RANGE)) return false;
//  if (!AD5933::setPGAGain(PGA_GAIN_X1)) return false;
//  if (!AD5933::setStartFrequency(START_FREQ)) return false;
//  if (!AD5933::setIncrementFrequency(FREQ_INCR)) return false;
//  if (!AD5933::setNumberIncrements(NUM_INCR)) return false;

//  // INCREASED SETTLING CYCLES FOR MORE STABLE MEASUREMENTS
//  if (!instance.setSettlingCycles(50)) {
//   Serial.println(F("Warning: Using default settling cycles"));
//  }

//  return true;
// }

// bool performCalibration() {
//  Serial.println(F("Calibrating with 470kΩ reference resistor..."));

//  // Initialize frequency sweep for calibration
//  if (!(AD5933::setPowerMode(POWER_STANDBY) &&
//     AD5933::setControlMode(CTRL_INIT_START_FREQ) &&
//     AD5933::setControlMode(CTRL_START_FREQ_SWEEP))) {
//   return false;
//  }

//  delay(100); // WAIT FOR INITIAL SETTLING

//  int point_count = 0;
//  unsigned long current_freq = START_FREQ;
//  double total_gain = 0.0;
//  int valid_points = 0;

//  // Collect calibration data point by point
//  while ((AD5933::readStatusRegister() & STATUS_SWEEP_DONE) != STATUS_SWEEP_DONE && point_count <= NUM_INCR) {

//   // WAIT FOR DATA READY
//   delay(50);

//   int real, imag;

//   if (!AD5933::getComplexData(&real, &imag)) {
//    Serial.print(F("Failed to get data at "));
//    Serial.println(current_freq);
//   } else {
//    double magnitude = sqrt((double)real * real + (double)imag * imag);

//    Serial.print(F("Cal point: "));
//    Serial.print(current_freq);
//    Serial.print(F(" Hz, Mag: "));
//    Serial.print(magnitude);
//    Serial.print(F(", Real: "));
//    Serial.print(real);
//    Serial.print(F(", Imag: "));
//    Serial.println(imag);

//    // Check for valid magnitude to avoid division by zero
//    if (magnitude > 1.0) {
//     double current_gain = 1.0 / (REF_RESIST * magnitude);
//     total_gain += current_gain;
//     valid_points++;

//     Serial.print(F(" -> Gain factor: "));
//     Serial.println(current_gain, 12);
//    } else {
//     Serial.print(F("Warning: Low magnitude at "));
//     Serial.print(current_freq);
//     Serial.print(F(" Hz: "));
//     Serial.println(magnitude);
//    }
//   }

//   point_count++;
//   current_freq += FREQ_INCR;

//   // PREVENT OVERFLOW
//   if (point_count > NUM_INCR) break;

//   AD5933::setControlMode(CTRL_INCREMENT_FREQ);
//   delay(10); // Small delay between increments
//  }

//  AD5933::setPowerMode(POWER_STANDBY);

//  Serial.print(F("Total points measured: "));
//  Serial.println(point_count);
//  Serial.print(F("Valid points for averaging: "));
//  Serial.println(valid_points);

//  // Calculate the average gain factor
//  if (valid_points > 0) {
//   gain_factor = total_gain / valid_points;
//   return true;
//  }

//  return false;
// }
// // The getGainFactor function is no longer needed since we have a single gain factor.
// // It is replaced with a direct use of the global gain_factor variable.
// void performImpedanceMeasurement() {
//  Serial.println(F("\nStarting measurement..."));
//  Serial.println(F("Frequency (Hz),Real Z (ohms),Imaginary Z (ohms),|Z| (ohms),Phase (degrees)"));

//  // Initialize frequency sweep
//  if (!(AD5933::setPowerMode(POWER_STANDBY) &&
//     AD5933::setControlMode(CTRL_INIT_START_FREQ) &&
//     AD5933::setControlMode(CTRL_START_FREQ_SWEEP))) {
//   Serial.println(F("ERROR: Failed to initialize sweep"));
//   return;
//  }

//  delay(100); // Initial settling
//  unsigned long current_freq = START_FREQ;
//  int point_count = 0;

//  // Collect and process data point by point
//  while ((AD5933::readStatusRegister() & STATUS_SWEEP_DONE) != STATUS_SWEEP_DONE && point_count <= NUM_INCR) {

//   delay(20); // Wait for data ready

//   int real, imag;

//   if (!AD5933::getComplexData(&real, &imag)) {
//    Serial.print(current_freq);
//    Serial.println(F(",ERROR,ERROR,ERROR,ERROR"));
//    current_freq += FREQ_INCR;
//    point_count++;
//    if (point_count <= NUM_INCR) {
//     AD5933::setControlMode(CTRL_INCREMENT_FREQ);
//    }
//    continue;
//   }

//   // Calculate magnitude and phase of DFT result
//   double magnitude = sqrt((double)real * real + (double)imag * imag);
//   double phase_rad = atan2(imag, real);
//   double phase_deg = phase_rad * 180.0 / PI;

//   // Apply calibration to get impedance
//   double impedance_magnitude = 0;

//   if (gain_factor > 0 && magnitude > 0) {
//    // CORRECTED impedance calculation per AD5933 datasheet
//    impedance_magnitude = 1.0 / (gain_factor * magnitude);
//   } else {
//    Serial.print(current_freq);
//    Serial.println(F(",ERROR,ERROR,ERROR,ERROR"));
//    current_freq += FREQ_INCR;
//    point_count++;
//    if (point_count <= NUM_INCR) {
//     AD5933::setControlMode(CTRL_INCREMENT_FREQ);
//    }
//    continue;
//   }

//   // Calculate real and imaginary components of impedance
//   double real_impedance = impedance_magnitude * cos(phase_rad);
//   double imag_impedance = impedance_magnitude * sin(phase_rad);

//   // VALIDATE RESULTS - flag suspicious values
//   bool suspicious = false;
//   if (real_impedance < 0 && abs(phase_deg) < 85) { // Real impedance shouldn't be negative for resistive samples
//    suspicious = true;
//   }
//   if (impedance_magnitude < 10 || impedance_magnitude > 1e8) { // Unreasonable magnitudes
//    suspicious = true;
//   }

//   // Output CSV data
//   Serial.print(current_freq);
//   Serial.print(F(","));
//   Serial.print(real_impedance, 6);
//   if (suspicious) Serial.print(F("*")); // Mark suspicious data
//   Serial.print(F(","));
//   Serial.print(imag_impedance, 6);
//   Serial.print(F(","));
//   Serial.print(impedance_magnitude, 6);
//   Serial.print(F(","));
//   Serial.println(phase_deg, 3);

//   current_freq += FREQ_INCR;
//   point_count++;

//   if (point_count <= NUM_INCR) {
//    AD5933::setControlMode(CTRL_INCREMENT_FREQ);
//   }
//  }

//  AD5933::setPowerMode(POWER_STANDBY);

//  Serial.println(F("\n=== Measurement Summary ==="));
//  Serial.print(F("Frequency range: "));
//  Serial.print(START_FREQ);
//  Serial.print(F(" Hz to "));
//  Serial.print(END_FREQ);
//  Serial.println(F(" Hz"));
//  Serial.print(F("Data points: "));
//  Serial.println(point_count);
//  Serial.print(F("Reference resistor: "));
//  Serial.print(REF_RESIST);
//  Serial.println(F(" ohms"));
//  Serial.println(F("Note: Suspicious values marked with *"));
//  Serial.println(F("==========================="));
// }









//v2.1 Corrected calibration for 10k ohm reference resistor
#include <Wire.h>
#include "AD5933.h"

// Frequency sweep parameters
#define START_FREQ      (1000)    // 1 kHz
#define END_FREQ        (100000)  // 100 kHz
#define FREQ_INCR       (2000)    // 2 kHz increments
#define NUM_INCR        ((END_FREQ - START_FREQ) / FREQ_INCR)
#define REF_RESIST      (12000)   // 120k ohm reference resistor - VERIFIED THIS IS CORRECT

// Small calibration table (store only every 5th point to save memory)
#define CAL_POINTS      20
double gain_table[CAL_POINTS];
unsigned long freq_table[CAL_POINTS];

// Output voltage range settings - CHANGED TO LOWER VOLTAGE FOR BETTER PERFORMANCE
#define OUTPUT_VOLTAGE_RANGE CTRL_OUTPUT_RANGE_3  // 0.4V p-p for better SNR with 470k

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
  Serial.println(F("ENSURE 12kΩ REFERENCE RESISTOR IS CONNECTED!"));
  delay(2000); // Give user time to read
  
  if (performCalibration()) {
    Serial.println(F("✓ Calibration completed!"));
    printCalibrationResults();
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
  
  // Clear calibration tables first
  for (int i = 0; i < CAL_POINTS; i++) {
    gain_table[i] = 0.0;
    freq_table[i] = 0;
  }
  
  // Initialize frequency sweep for calibration
  if (!(AD5933::setPowerMode(POWER_STANDBY) &&
        AD5933::setControlMode(CTRL_INIT_START_FREQ) &&
        AD5933::setControlMode(CTRL_START_FREQ_SWEEP))) {
    return false;
  }

  // WAIT FOR INITIAL SETTLING
  delay(100);

  int cal_index = 0;
  int point_count = 0;
  unsigned long current_freq = START_FREQ;

  // Collect calibration data point by point
  while ((AD5933::readStatusRegister() & STATUS_SWEEP_DONE) != STATUS_SWEEP_DONE && point_count <= NUM_INCR) {
    
    // WAIT FOR DATA READY
    delay(50);
    
    int real, imag;
    
    if (!AD5933::getComplexData(&real, &imag)) {
      Serial.print(F("Failed to get data at "));
      Serial.println(current_freq);
      current_freq += FREQ_INCR;
      point_count++;
      AD5933::setControlMode(CTRL_INCREMENT_FREQ);
      continue;
    }

    // Store every 5th point to reduce memory usage
    if (point_count % 5 == 0 && cal_index < CAL_POINTS) {
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
      if (magnitude > 1.0) {  // Lowered threshold for 10k resistor
        // CORRECTED gain factor calculation
        // For AD5933: |Z| = 1/(Gain_Factor × DFT_Code)
        // Therefore: Gain_Factor = 1/(|Z| × DFT_Code)
        gain_table[cal_index] = 1.0 / (REF_RESIST * magnitude);
        freq_table[cal_index] = current_freq;
        
        Serial.print(F("  -> Gain factor: "));
        Serial.println(gain_table[cal_index], 12);
        
        cal_index++;
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
  Serial.print(F("Calibration points stored: "));
  Serial.println(cal_index);
  
  // Validate calibration results
  if (cal_index < 2) {
    Serial.println(F("ERROR: Insufficient calibration points!"));
    return false;
  }
  
  // Check for reasonable gain factor values (adjusted for 10k resistor)
  for (int i = 0; i < cal_index; i++) {
    if (gain_table[i] < 1e-15 || gain_table[i] > 1e-3) {
      Serial.print(F("Warning: Unusual gain factor at index "));
      Serial.print(i);
      Serial.print(F(" ("));
      Serial.print(freq_table[i]);
      Serial.print(F(" Hz): "));
      Serial.println(gain_table[i], 12);
    }
  }
  
  return (cal_index > 1); // Need at least 2 points for interpolation
}

// Print calibration results for verification
void printCalibrationResults() {
  Serial.println(F("\n=== Calibration Results ==="));
  Serial.println(F("Frequency (Hz), Gain Factor, Expected |Z| (ohms)"));
  for (int i = 0; i < CAL_POINTS && freq_table[i] > 0; i++) {
    double expected_z = REF_RESIST; // Should be close to 10k for resistor
    Serial.print(freq_table[i]);
    Serial.print(F(", "));
    Serial.print(gain_table[i], 12);
    Serial.print(F(", "));
    Serial.println(expected_z, 1);
  }
  Serial.println(F("========================\n"));
}

// Linear interpolation to get gain factor for any frequency
double getGainFactor(unsigned long frequency) {
  // Find the number of valid calibration points
  int valid_points = 0;
  for (int i = 0; i < CAL_POINTS; i++) {
    if (freq_table[i] > 0) valid_points++;
    else break;
  }
  
  if (valid_points == 0) return 0.0;
  if (valid_points == 1) return gain_table[0];
  
  // Find the two closest calibration points
  for (int i = 0; i < valid_points - 1; i++) {
    if (frequency >= freq_table[i] && frequency <= freq_table[i + 1]) {
      // Linear interpolation
      double ratio = (double)(frequency - freq_table[i]) / (freq_table[i + 1] - freq_table[i]);
      return gain_table[i] + ratio * (gain_table[i + 1] - gain_table[i]);
    }
  }
  
  // If outside range, use nearest point
  if (frequency < freq_table[0]) return gain_table[0];
  return gain_table[valid_points - 1];
}

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
    double gain_factor = getGainFactor(current_freq);
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











// //v2.0 improved calibration sweep
// #include <Wire.h>
// #include "AD5933.h"

// // Frequency sweep parameters
// #define START_FREQ      (1000)    // 1 kHz
// #define END_FREQ        (100000)  // 100 kHz
// #define FREQ_INCR       (2000)    // 2 kHz increments
// #define NUM_INCR        ((END_FREQ - START_FREQ) / FREQ_INCR)
// #define REF_RESIST      (10000)   // 10k ohm reference resistor

// // Small calibration table (store only every 5th point to save memory)
// #define CAL_POINTS      20
// double gain_table[CAL_POINTS];
// unsigned long freq_table[CAL_POINTS];

// // Output voltage range settings
// #define OUTPUT_VOLTAGE_RANGE CTRL_OUTPUT_RANGE_3  // 0.4V p-p (default)

// void setup(void) {
//   Wire.begin();
//   Serial.begin(115200);
  
//   while (!Serial) {
//     delay(10);
//   }
  
//   Serial.println(F("AD5933 Enhanced Impedance Analyzer"));
//   Serial.println(F("==================================="));

//   if (!AD5933::reset()) {
//     Serial.println(F("ERROR: Failed to reset AD5933!"));
//     while (true);
//   }
//   Serial.println(F("✓ AD5933 reset successful"));

//   if (!configureAD5933()) {
//     Serial.println(F("ERROR: Failed to configure AD5933!"));
//     while (true);
//   }
//   Serial.println(F("✓ AD5933 configured successfully"));

//   Serial.println(F("\nStarting calibration..."));
//   if (performCalibration()) {
//     Serial.println(F("✓ Calibration completed!"));
//     printCalibrationResults();
//   } else {
//     Serial.println(F("ERROR: Calibration failed!"));
//     while (true);
//   }

//   Serial.println(F("\n==================================="));
//   Serial.println(F("Setup complete!"));
//   Serial.println(F("Connect test impedance and send any character"));
//   Serial.println(F("===================================\n"));
// }

// void loop(void) {
//   if (Serial.available() > 0) {
//     while (Serial.available() > 0) {
//       Serial.read();
//     }
    
//     performImpedanceMeasurement();
//     Serial.println(F("\nSend any character for another measurement."));
//   }
  
//   delay(100);
// }

// bool configureAD5933() {
//   if (!AD5933::setInternalClock(true)) return false;
  
//   AD5933 instance;
//   if (!instance.setRange(OUTPUT_VOLTAGE_RANGE)) return false;
//   if (!AD5933::setPGAGain(PGA_GAIN_X1)) return false;
//   if (!AD5933::setStartFrequency(START_FREQ)) return false;
//   if (!AD5933::setIncrementFrequency(FREQ_INCR)) return false;
//   if (!AD5933::setNumberIncrements(NUM_INCR)) return false;
  
//   if (!instance.setSettlingCycles(15)) {
//     Serial.println(F("Warning: Using default settling cycles"));
//   }
  
//   return true;
// }

// bool performCalibration() {
//   Serial.println(F("Calibrating with reference resistor..."));
  
//   // Initialize frequency sweep for calibration
//   if (!(AD5933::setPowerMode(POWER_STANDBY) &&
//         AD5933::setControlMode(CTRL_INIT_START_FREQ) &&
//         AD5933::setControlMode(CTRL_START_FREQ_SWEEP))) {
//     return false;
//   }

//   int cal_index = 0;
//   int point_count = 0;
//   unsigned long current_freq = START_FREQ;

//   // Collect calibration data point by point
//   while ((AD5933::readStatusRegister() & STATUS_SWEEP_DONE) != STATUS_SWEEP_DONE) {
//     int real, imag;
    
//     if (!AD5933::getComplexData(&real, &imag)) {
//       return false;
//     }

//     // Store every 5th point to reduce memory usage
//     if (point_count % 5 == 0 && cal_index < CAL_POINTS) {
//       double magnitude = sqrt((double)real * real + (double)imag * imag);
      
//       // Check for valid magnitude to avoid division by zero
//       if (magnitude > 10.0) {  // Minimum threshold for valid reading
//         // Corrected gain factor calculation per AD5933 datasheet
//         // Gain Factor = 1 / (DFT_Magnitude × Reference_Impedance)
//         // This gives us (1/ohm) units which when multiplied by DFT magnitude gives impedance
//         gain_table[cal_index] = 1.0 / (magnitude * REF_RESIST);
//         freq_table[cal_index] = current_freq;
//         cal_index++;
//       } else {
//         Serial.print(F("Warning: Low magnitude at "));
//         Serial.print(current_freq);
//         Serial.print(F(" Hz: "));
//         Serial.println(magnitude);
//       }
//     }

//     point_count++;
//     current_freq += FREQ_INCR;
//     AD5933::setControlMode(CTRL_INCREMENT_FREQ);
//   }

//   AD5933::setPowerMode(POWER_STANDBY);
  
//   Serial.print(F("Calibration points stored: "));
//   Serial.println(cal_index);
  
//   // Validate calibration results
//   if (cal_index < 2) {
//     Serial.println(F("ERROR: Insufficient calibration points!"));
//     return false;
//   }
  
//   // Check for reasonable gain factor values
//   for (int i = 0; i < cal_index; i++) {
//     if (gain_table[i] < 1e-12 || gain_table[i] > 1e-6) {
//       Serial.print(F("Warning: Unusual gain factor at index "));
//       Serial.print(i);
//       Serial.print(F(": "));
//       Serial.println(gain_table[i], 12);
//     }
//   }
  
//   return (cal_index > 0);
// }

// // Print calibration results for verification
// void printCalibrationResults() {
//   Serial.println(F("\n=== Calibration Results ==="));
//   Serial.println(F("Frequency (Hz), Gain Factor"));
//   for (int i = 0; i < CAL_POINTS && freq_table[i] > 0; i++) {
//     Serial.print(freq_table[i]);
//     Serial.print(F(", "));
//     Serial.println(gain_table[i], 12);
//   }
//   Serial.println(F("========================\n"));
// }

// // Linear interpolation to get gain factor for any frequency
// double getGainFactor(unsigned long frequency) {
//   // Find the number of valid calibration points
//   int valid_points = 0;
//   for (int i = 0; i < CAL_POINTS; i++) {
//     if (freq_table[i] > 0) valid_points++;
//     else break;
//   }
  
//   if (valid_points == 0) return 0.0;
//   if (valid_points == 1) return gain_table[0];
  
//   // Find the two closest calibration points
//   for (int i = 0; i < valid_points - 1; i++) {
//     if (frequency >= freq_table[i] && frequency <= freq_table[i + 1]) {
//       // Linear interpolation
//       double ratio = (double)(frequency - freq_table[i]) / (freq_table[i + 1] - freq_table[i]);
//       return gain_table[i] + ratio * (gain_table[i + 1] - gain_table[i]);
//     }
//   }
  
//   // If outside range, use nearest point
//   if (frequency < freq_table[0]) return gain_table[0];
//   return gain_table[valid_points - 1];
// }

// void performImpedanceMeasurement() {
//   Serial.println(F("\nStarting measurement..."));
//   Serial.println(F("Frequency (Hz),Real Z (ohms),Imaginary Z (ohms),|Z| (ohms),Phase (degrees)"));
  
//   // Initialize frequency sweep
//   if (!(AD5933::setPowerMode(POWER_STANDBY) &&
//         AD5933::setControlMode(CTRL_INIT_START_FREQ) &&
//         AD5933::setControlMode(CTRL_START_FREQ_SWEEP))) {
//     Serial.println(F("ERROR: Failed to initialize sweep"));
//     return;
//   }

//   unsigned long current_freq = START_FREQ;
  
//   // Collect and process data point by point
//   while ((AD5933::readStatusRegister() & STATUS_SWEEP_DONE) != STATUS_SWEEP_DONE) {
//     int real, imag;
    
//     if (!AD5933::getComplexData(&real, &imag)) {
//       Serial.println(F("ERROR: Failed to get data"));
//       return;
//     }

//     // Calculate magnitude and phase of DFT result
//     double magnitude = sqrt((double)real * real + (double)imag * imag);
//     double phase_rad = atan2(imag, real);
//     double phase_deg = phase_rad * 180.0 / PI;
    
//     // Apply calibration to get impedance
//     double gain_factor = getGainFactor(current_freq);
//     double impedance_magnitude = 0;
    
//     if (gain_factor > 0 && magnitude > 0) {
//       // Corrected impedance calculation per AD5933 datasheet
//       // Impedance = 1 / (Gain_Factor × DFT_Magnitude)
//       impedance_magnitude = 1.0 / (gain_factor * magnitude);
//     } else {
//       Serial.print(F("Warning: Invalid calibration at "));
//       Serial.print(current_freq);
//       Serial.println(F(" Hz"));
//     }
    
//     // Calculate real and imaginary components of impedance
//     double real_impedance = impedance_magnitude * cos(phase_rad);
//     double imag_impedance = impedance_magnitude * sin(phase_rad);
    
//     // Output CSV data
//     Serial.print(current_freq);
//     Serial.print(F(","));
//     Serial.print(real_impedance, 6);
//     Serial.print(F(","));
//     Serial.print(imag_impedance, 6);
//     Serial.print(F(","));
//     Serial.print(impedance_magnitude, 6);
//     Serial.print(F(","));
//     Serial.println(phase_deg, 3);
    
//     current_freq += FREQ_INCR;
//     AD5933::setControlMode(CTRL_INCREMENT_FREQ);
//   }

//   AD5933::setPowerMode(POWER_STANDBY);
  
//   Serial.println(F("\n=== Measurement Summary ==="));
//   Serial.print(F("Frequency range: "));
//   Serial.print(START_FREQ);
//   Serial.print(F(" Hz to "));
//   Serial.print(END_FREQ);
//   Serial.println(F(" Hz"));
//   Serial.print(F("Data points: "));
//   Serial.println(NUM_INCR + 1);
//   Serial.print(F("Reference resistor: "));
//   Serial.print(REF_RESIST);
//   Serial.println(F(" ohms"));
//   Serial.println(F("==========================="));
// }















//v1.0
// #include <Wire.h>
// #include "AD5933.h"

// // Frequency sweep parameters
// #define START_FREQ      (5000)    // 5 kHz
// #define END_FREQ        (1000000)  // 1000 kHz
// #define FREQ_INCR       (10000)    // 10 kHz increments
// #define NUM_INCR        ((END_FREQ - START_FREQ) / FREQ_INCR)
// #define REF_RESIST      (470000)   // 470k ohm reference resistor

// // Small calibration table (store only every 5th point to save memory)
// #define CAL_POINTS      20
// double gain_table[CAL_POINTS];
// unsigned long freq_table[CAL_POINTS];

// // Output voltage range settings
// #define OUTPUT_VOLTAGE_RANGE CTRL_OUTPUT_RANGE_3  // 0.4V p-p (default)

// void setup(void) {
//   Wire.begin();
//   Serial.begin(115200);
  
//   while (!Serial) {
//     delay(10);
//   }
  
//   Serial.println(F("AD5933 Enhanced Impedance Analyzer"));
//   Serial.println(F("==================================="));

//   if (!AD5933::reset()) {
//     Serial.println(F("ERROR: Failed to reset AD5933!"));
//     while (true);
//   }
//   Serial.println(F("✓ AD5933 reset successful"));

//   if (!configureAD5933()) {
//     Serial.println(F("ERROR: Failed to configure AD5933!"));
//     while (true);
//   }
//   Serial.println(F("✓ AD5933 configured successfully"));

//   Serial.println(F("\nStarting calibration..."));
//   if (performCalibration()) {
//     Serial.println(F("✓ Calibration completed!"));
//   } else {
//     Serial.println(F("ERROR: Calibration failed!"));
//     while (true);
//   }

//   Serial.println(F("\n==================================="));
//   Serial.println(F("Setup complete!"));
//   Serial.println(F("Connect test impedance and send any character"));
//   Serial.println(F("===================================\n"));
// }

// void loop(void) {
//   if (Serial.available() > 0) {
//     while (Serial.available() > 0) {
//       Serial.read();
//     }
    
//     performImpedanceMeasurement();
//     Serial.println(F("\nSend any character for another measurement."));
//   }
  
//   delay(100);
// }

// bool configureAD5933() {
//   if (!AD5933::setInternalClock(true)) return false;
  
//   AD5933 instance;
//   if (!instance.setRange(OUTPUT_VOLTAGE_RANGE)) return false;
//   if (!AD5933::setPGAGain(PGA_GAIN_X1)) return false;
//   if (!AD5933::setStartFrequency(START_FREQ)) return false;
//   if (!AD5933::setIncrementFrequency(FREQ_INCR)) return false;
//   if (!AD5933::setNumberIncrements(NUM_INCR)) return false;
  
//   if (!instance.setSettlingCycles(15)) {
//     Serial.println(F("Warning: Using default settling cycles"));
//   }
  
//   return true;
// }

// bool performCalibration() {
//   Serial.println(F("Calibrating with reference resistor..."));
  
//   // Initialize frequency sweep for calibration
//   if (!(AD5933::setPowerMode(POWER_STANDBY) &&
//         AD5933::setControlMode(CTRL_INIT_START_FREQ) &&
//         AD5933::setControlMode(CTRL_START_FREQ_SWEEP))) {
//     return false;
//   }

//   int cal_index = 0;
//   int point_count = 0;
//   unsigned long current_freq = START_FREQ;

//   // Collect calibration data point by point
//   while ((AD5933::readStatusRegister() & STATUS_SWEEP_DONE) != STATUS_SWEEP_DONE) {
//     int real, imag;
    
//     if (!AD5933::getComplexData(&real, &imag)) {
//       return false;
//     }

//     // Store every 5th point to reduce memory usage
//     if (point_count % 5 == 0 && cal_index < CAL_POINTS) {
//       double magnitude = sqrt((double)real * real + (double)imag * imag);
      
//       if (magnitude > 0) {
//         // gain_table[cal_index] = (double)REF_RESIST / magnitude;
//         gain_table[cal_index] = 1.0 / (magnitude * REF_RESIST);
//         freq_table[cal_index] = current_freq;
//         cal_index++;
//       }
//     }

//     point_count++;
//     current_freq += FREQ_INCR;
//     AD5933::setControlMode(CTRL_INCREMENT_FREQ);
//   }

//   AD5933::setPowerMode(POWER_STANDBY);
  
//   Serial.print(F("Calibration points stored: "));
//   Serial.println(cal_index);
  
//   return (cal_index > 0);
// }

// // Linear interpolation to get gain factor for any frequency
// double getGainFactor(unsigned long frequency) {
//   // Find the two closest calibration points
//   for (int i = 0; i < CAL_POINTS - 1; i++) {
//     if (frequency >= freq_table[i] && frequency <= freq_table[i + 1]) {
//       // Linear interpolation
//       double ratio = (double)(frequency - freq_table[i]) / (freq_table[i + 1] - freq_table[i]);
//       return gain_table[i] + ratio * (gain_table[i + 1] - gain_table[i]);
//     }
//   }
  
//   // If outside range, use nearest point
//   if (frequency < freq_table[0]) return gain_table[0];
//   return gain_table[CAL_POINTS - 1];
// }

// void performImpedanceMeasurement() {
//   Serial.println(F("\nStarting measurement..."));
//   Serial.println(F("Frequency (Hz),Real Z (ohms),Imaginary Z (ohms),|Z| (ohms),Phase (degrees)"));
  
//   // Initialize frequency sweep
//   if (!(AD5933::setPowerMode(POWER_STANDBY) &&
//         AD5933::setControlMode(CTRL_INIT_START_FREQ) &&
//         AD5933::setControlMode(CTRL_START_FREQ_SWEEP))) {
//     Serial.println(F("ERROR: Failed to initialize sweep"));
//     return;
//   }

//   unsigned long current_freq = START_FREQ;
  
//   // Collect and process data point by point
//   while ((AD5933::readStatusRegister() & STATUS_SWEEP_DONE) != STATUS_SWEEP_DONE) {
//     int real, imag;
    
//     if (!AD5933::getComplexData(&real, &imag)) {
//       Serial.println(F("ERROR: Failed to get data"));
//       return;
//     }

//     // Calculate magnitude and phase
//     double magnitude = sqrt((double)real * real + (double)imag * imag);
//     double phase_rad = atan2(imag, real);
//     double phase_deg = phase_rad * 180.0 / PI;
    
//     // Apply calibration
//     double gain_factor = getGainFactor(current_freq);
//     double impedance_magnitude = 0;
    
//     if (gain_factor > 0 && magnitude > 0) {
//       // impedance_magnitude = 1.0 / (gain_factor / REF_RESIST * magnitude / REF_RESIST);
//       impedance_magnitude = 1.0 / (gain_factor * magnitude);
//     }
    
//     // Calculate real and imaginary components
//     double real_impedance = impedance_magnitude * cos(phase_rad);
//     double imag_impedance = impedance_magnitude * sin(phase_rad);
    
//     // Output CSV data
//     Serial.print(current_freq);
//     Serial.print(F(","));
//     Serial.print(real_impedance, 6);
//     Serial.print(F(","));
//     Serial.println(imag_impedance, 6);
//     Serial.print(F(","));
//     Serial.print(impedance_magnitude, 6);
//     Serial.print(F(","));
//     Serial.print(phase_deg, 3);
    
//     current_freq += FREQ_INCR;
//     AD5933::setControlMode(CTRL_INCREMENT_FREQ);
//   }

//   AD5933::setPowerMode(POWER_STANDBY);
  
//   Serial.println(F("\n=== Measurement Summary ==="));
//   Serial.print(F("Frequency range: "));
//   Serial.print(START_FREQ);
//   Serial.print(F(" Hz to "));
//   Serial.print(END_FREQ);
//   Serial.println(F(" Hz"));
//   Serial.print(F("Data points: "));
//   Serial.println(NUM_INCR + 1);
//   Serial.println(F("==========================="));
// }

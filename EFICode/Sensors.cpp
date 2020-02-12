#include "Controller.h"
#include "Arduino.h"
#include "Constants.h"
#include "math.h"

// timepassed is in microseconds
long Controller::getRPM (long int timePassed, int rev) {
  return (60 * 1E6 * rev) / (timePassed);
}

//TPS Measurement
/*
const double TPSConversion = .0019685;
const double TPSOffset = -.33746;
*/
const double TPS_0Deg = 60;
const double TPS_90Deg = 865;

double Controller::getTPS() {
  unsigned long currThrottleMeasurementTime = micros();
  // calculate open throttle area (i think)
  double newTPS = 1 - cos(((double(analogRead(TPS_Pin))-TPS_0Deg)/(TPS_90Deg - TPS_0Deg))*HALF_PI);
  
  if(newTPS < 0)
    newTPS = 0;
  if(newTPS > 1)
    newTPS = 1;
  if(currThrottleMeasurementTime - lastThrottleMeasurementTime > 0)
    DTPS = (newTPS - TPS) / (currThrottleMeasurementTime - lastThrottleMeasurementTime);
  lastThrottleMeasurementTime = currThrottleMeasurementTime;
  return newTPS;
}

double Controller::computeThrottleAdjustment() { // SHOULD THIS INCORPORATE DTPS?
  // Looks at the change in throttle position and determines a proper adjusment for the fuel input.
  // DTPS
  return 1 + TPS * TPS;
}

// Identify the constant for each temperature sensor
const int IAT_INDEX = 0;
const int ECT_INDEX = 1;

//The following constants are to complete the following eq for temperature
//
// Temp = tempBeta / (ln(R) + (tempBeta/T_0 - lnR_0))
//	where R is the resistance of the sensor (found using voltage divider)
//	eq from: https://en.wikipedia.org/wiki/Thermistor#B_or_%CE%B2_parameter_equation
//
const double tempBeta[2] = {3988,3988}; // tolerance: {+/-1%,+/-1.5%}
const double T_0 = 298.15; // temp in Kelvin at which R_0 values are taken
const double lnR_0[2] = {9.21034,8.4849};//8.45531}; // {ln(10000 (10000 +/-1%)),ln(4700 (4559 to 4841))}
const double tempConst[2] = {tempBeta[IAT_INDEX]/T_0 - lnR_0[IAT_INDEX], tempBeta[ECT_INDEX]/T_0 - lnR_0[ECT_INDEX]};
const double R_div[2] = {9300,10000}; // resistance of other resistor in voltage divider

double Controller::getTemp(int pin) {
  // TODO: Create log lookup table.

  int index; // identify which constants to use
  switch(pin) {
    case IAT_Pin: index = IAT_INDEX;
    break;
    case ECT_Pin: index = ECT_INDEX;
    break;
    default:  index = ECT_INDEX; // just in case
  }
  double tempR = R_div[index] / (maxADC/analogRead(pin) - 1); // find resistance of sensor
  return tempBeta[index] / (log(tempR) + tempConst[index]);   // return temperature
}

//MAP Measurement

// MPX4115A MAP sensor calibration
const double MAPVs = Vs_5;
const double MAPDelta = 0.045; // should be between +/- 0.0675 volts (1.5 * 0.009 * Vs where Vs is 5)
const double MAPSlope = 1E3/(MAPVs*0.009);  //Pa / Volt
const double MAPOffset = 1E3*MAPDelta/(MAPVs*0.009) + 1E3*0.095/0.009;   //Pa
const double MAPConversion = MAPSlope * adcToOpampVin;    // Pascals / 1023

double Controller::getMAP() {
  //Calculates MAP, outputs in Pa
  return MAPConversion * analogRead(MAP_Pin) + MAPOffset;
}

// Analog output 1 factory default settings for voltage ranges.
const double AO1minAFR = 7.35;     //grams air to grams fuel at zero Volts
const double AO1maxAFR = 22.39;    //grams air to grams fuel at five Volts
const double AO1slope = (AO1maxAFR - AO1minAFR) / (5 - 0);

// Analog output 2 factory default settings for voltage ranges
const double AO2minAFR = 14;    //grams air to grams fuel at 0.1 Volts
const double AO2maxAFR = 15;    //grams air to grams fuel at 1.1 Volts
const double AO2slope = (AO2maxAFR - AO2minAFR) / (1.1 - 0.1);

// IF O2 SENSOR IS ERRORING OR NOT READY, THE ANALOG OUTPUT IS SET TO BE EQUAL
// TO ZERO VOLTS. THEREFORE, I HAVE IMPOSED A 0.05 Volt LIMITATION ON VOLTAGES READ
// FROM THE O2 SENSOR. IF THE VOLTAGE READ IS LESS THAN 0.05 Volts, then the AFR
// FEEDBACK LOOP WILL DO NOTHING! 

// Returns the current measured AFR.
double Controller::getAFR () {
  // Gets Reading from O2 Sensor.
  
  // Calculate initial AFR reading.
  AFRVolts->addData(adcToOpampVin * analogRead(OIN1_Pin));
  AFR = AFRVolts->getData() * AO1slope + AO1minAFR;
  
  // If AFR is close to stoich, use narrow band output with greater precision.
//  if (AFR <= 15 && AFR >= 14) {
//      AFR = voltageConversion * analogRead(OIN2_Pin) * AO2slope + AO2minAFR;
//  }
  
  return AFR;
}

#include "TimerThree.h"
#include "Constants.h"
#include "Pulse.h"

#include "Runner.h"

Controller *c;
void countRev();
void pulseOff();

void setup() {
  c = new Controller();
  c -> readSensors();
  attachInterrupt(HES_Pin, countRev, FALLING);
  c->calculatePulseTime();
  Timer3.initialize(100000);
  Timer3.attachInterrupt(pulseOff, pulseTime); //Attaches timer-based interrupt to stop injector from injecting

  //TODO: Neaten this
  c->realPulseTime = 0;
  c->lastRPMCalcTime = millis();
  lastInterrupt = millis();
}

void loop() {
  //TODO: Abstract this away
  if (c->RPM > c->endRPM) {
    //If the engine goes above our max RPM, shut it off and print out the following analytics
    c->fuelRatio = 0;
    c->openTime = 0;
    c->printEndingData();
  }
  if (revolutions >= 25) {
    //Calculate RPM each cycle
    c->RPM = c->getRPM(micros() - c->lastRPMCalcTime, revolutions);
    c->startupVal = 1;
    c->revolutions = 0;
    c->lastRPMCalculationTime = micros();
  }

  c->calculatePulseTime(); //Calculate injection time on each loop cycle
  
  if (micros() - c->lastRPMCalcTime >= 2000000) {
    //If RPM falls below 600, sets RPM to 0 meaning shut the engine off
    c->RPM = 0;
    c->startupVal = resetVal;
    c->totalRevolutions = 0;
  }
  if (micros()-c->lastSerialOutputTime>=500000) {
    //If it's been more than 500ms since we last printed data, print data
    c->printCurrentData();
    c->lastSerialOutputTime = micros();
  }
}

void countRev() {
  c->countRevolutions();
}

void pulseOff() {
  //When it's time to turn the injector off, follow these steps and turn it off
  //ISR for Timer3 interrupt
  if (c->delayCount == 1) {
    //Determined experimentally.  I think the Timer interrupts in the very beginning.  To avoid this first interrupt, I switched to 1.  Can probably be revised and improved
    digitalWrite(INJ_Pin, LOW);
    c->realPulseTime = (micros() - lastRPMCalcTime) / 1000000;
    int arrayIndex = c->RPM / RPMIncrement;
    c->totalPulse[arrayIndex] += c->pulseTimeReal;
  }
  if (delayCount > 100) {
    c->delayCount = 2;
  }
   c->delayCount++;
}
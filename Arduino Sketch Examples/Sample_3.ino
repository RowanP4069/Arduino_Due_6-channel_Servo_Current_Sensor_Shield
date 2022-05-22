/* Arduino Due 6-channel servo current sensor Sample_3.ino
  by Rowan Patterson

  A progression on the example of Sample_2.ino, to now include multiple sensors.
  The sketch is one of a series to test a custom servo current sensor shield for 
  Arduino Due constructed from ACS712 5A current sensors multiplexed through an 
  74HC4051 IC. 

  CUSTOMISATION NOTE
    The running average and EWMA calculations are independent. They're both shown 
    for the purposes of demonstration and to maintain a progression from the previous
    samples. The code could be cleaned up if one or the other was not required.

  ACKNOWLEDGEMENTS
    Multiplexor approach is based on Mux_Analog_Input.ino SparkFun Multiplexer Analog
    Input Example by Jim Lindblom @ SparkFun Electronics August 15, 2016
    https://github.com/sparkfun/74HC4051_8-Channel_Mux_Breakout
    
    Running Average smoothing approach is based on official arduino example
    https://www.arduino.cc/en/Tutorial/BuiltInExamples/Smoothing
    
    Exponential Weighted Moving Average (EWMA) approach is based on the sample sketch
    by Jon Froehlich 
    https://github.com/makeabilitylab/arduino/blob/master/Filters/ExponentialWeightedMovingAverageFilter/ExponentialWeightedMovingAverageFilter.ino 

  LICENCE
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
  
    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.
  
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

  PIN USAGE:
    Shield JP1+ - Servo 1 connector, power supply / MCU side
    Shield JP1- - Servo 1 connector, servo side
    Shield JP2 through 6 +&- - as above for remaining servos.
    Due A8 (input) - to 74HC4051 Common (Z) (level shifted)
    Due D2 (output)- to 74HC4051 (S0 / A) selector
    Due D3 (output)- to 74HC4051 (S1 / B) selector
    Due D4 (output)- to 74HC4051 (S2 / C) selector
    Due 5V - to 74HC4051 Vcc
    Due 5V - to ACS712 Vcc
    ACS712 VIout (1 through 6) - to 74HC4051 (Y1 through Y6)
  */

//Constants
  const int selectPins[3] = {2, 3, 4}; // Which arduino digital pins are connected to 74HC4051 selector pins
  const int zInput = A8; //Which arduino analog pin is connected to 74HC4051 common I/O pin
  const int sampleDelay = 100; //milliseconds between sensor readings
  const float r1 = 2400.0; //Value of R1 (ohms) in level shift voltage divider
  const float r2 = 4700.0; //Value of R1 (ohms) in level shift voltage divider
  const float dueRefVoltage = 3.3; //Ref. https://www.arduino.cc/reference/en/language/functions/analog-io/analogreference/ 
  const int dueNumSamples = 1024; //Ref. https://www.arduino.cc/reference/en/language/functions/analog-io/analogread/ 
  const float vioutq = 2.26; // VIout(Q) value of ACS712 sensor. The zero current output voltage.
                             // NB. vioutq(observed) = 2.26V; vioutq(calculated) = 1.65V 
  const float sensitivity = 0.185; // ACS712 sensitivity in mV/A 
  const int numReadings = 10; // Number of readings in the moving average calculation window
  const float ewmaAlpha = 0.1; // The exponential weighted moving average (EWMA) alpha value. Towards 0 = weight previous readings; towards 1 = weight newest reading
  const int numMuxPins = 6; 
  const int muxPin [numMuxPins] = {1, 2, 3, 4, 5, 6}; //Which mux pin is each sensor connected to. Note, no connection on pin 0 or 8 for the selected shield.
//Enumerations and Types
  enum pinIndex { // more easily reference indices of the results array by pin number
    pin1,         // n.b., pin index equals pin number -1
    pin2, 
    pin3, 
    pin4, 
    pin5, 
    pin6
  };
//Variables
  float readingAmps [numMuxPins] [numReadings] = {0}; // The calculated ACS712 current reading (Ip), for each sensor/pin, stored in a revolving buffer for averaging & totalling.
  int readIndex =0; // The index into the revolving buffer used to store sensor readings. 
  float readingTotal [numMuxPins] = {0.0}; // The total of the readings in the revolving buffer
  float readingAverage [numMuxPins] = {0.0}; // The average of the readings in the revolving buffer
  double readingEwma [numMuxPins] = {0.0}; // The  Exponential Weighted Moving Average (EWMA) for each sensor

void setup() {
  Serial.begin(115200); // Initialize the serial port
  // Set up the select pins as outputs:
  for (int i=0; i<3; i++) {
    pinMode(selectPins[i], OUTPUT);
    digitalWrite(selectPins[i], HIGH);
  }
  pinMode(zInput, INPUT); // Set up Z as an input
  for (int i=pin1; i<=pin6; i++) { // For each of the sensors connected to the mux ...
    selectMuxPin(muxPin[i]); // Select the sensor to be read
    readingEwma[i] = (vioutq - (float(analogRead(zInput)) * dueRefVoltage / float(dueNumSamples) * (r1+r2) / r2)) / sensitivity; // and make an initial reading for EWMA index 1
  }
}

void loop() {
  for (int i=pin1; i<=pin6; i++) { // For each of the sensors connected to the mux ...
    selectMuxPin(muxPin[i]); // Select the sensor to be read
    readingTotal[i] = readingTotal[i] - readingAmps[i] [readIndex]; //Deduct the oldest reading from the sensor total
    readingAmps[i][readIndex] = (vioutq - (float(analogRead(zInput)) * dueRefVoltage / float(dueNumSamples) * (r1+r2) / r2)) / sensitivity; //Make a new reading
    readingTotal[i] = readingTotal[i] + readingAmps[i][readIndex]; // Add the new reading to the sensor total
    readingAverage[i] = readingTotal[i] / numReadings;   // calculate the running average 
    readingEwma[i] = (ewmaAlpha * readingAmps[i][readIndex]) + (1-ewmaAlpha) * readingEwma[i];
    Serial.print(String(muxPin[i]) + "Ip(ewma):" + String(readingEwma[i]) + ","); // Output for Arduino IDE Serial Plotter 
  }
  Serial.println();
  readIndex = readIndex + 1; // advance to the next position in the revolving buffer
  if (readIndex >= numReadings) {  // if we're at the end of the array...
    readIndex = 0; // ...wrap around to the beginning
  }
  delay(sampleDelay); //Blocking code for simple example
}

void selectMuxPin(byte pin) {
  // The selectMuxPin function sets the S0, S1, and S2 pins
  // accordingly, given a 74HC4051 multiplexer pin from 0-7.
  for (int i=0; i<3; i++) {
    if (pin & (1<<i)) //Compare the ith bit of pin to a 1 bit
      digitalWrite(selectPins[i], HIGH);
    else
      digitalWrite(selectPins[i], LOW);
  }
}
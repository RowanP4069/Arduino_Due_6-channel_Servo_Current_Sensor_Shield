/* Arduino Due 6-channel servo current sensor Sample_1.ino
  by Rowan Patterson

  Simplest example sketch - Read one sensor only, blocking code (i.e., uses delay() ), unsmoothed (i.e., no noise filtering)

	An Arduino Due sketch to test a custom servo current sensor shield constructed from ACS712 5A current sensors
  multiplexed through an 74HC4051 IC. 

  Sketch is based on Mux_Analog_Input.ino SparkFun Multiplexer Analog Input Example by Jim Lindblom @ SparkFun Electronics August 15, 2016
  https://github.com/sparkfun/74HC4051_8-Channel_Mux_Breakout

	This library is free software; you can redistribute it and/or
 	modify it under the terms of the GNU Lesser General Public
 	License as published by the Free Software Foundation; either
 	version 2.1 of the License, or (at your option) any later version.
 	This library is distributed in the hope that it will be useful,
 	but WITHOUT ANY WARRANTY; without even the implied warranty of
 	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 	Lesser General Public License for more details.
 	You should have received a copy of the GNU Lesser General Public
 	License along with this library; if not, write to the Free Software
 	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  Pin usage:
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
  const int numSamplesPerSec = int(1000/(sampleDelay));
  const int pin = 1; //Selected sensor for the example
  const int r1 = 2400; //Value of R1 (ohms) in level shift voltage divider
  const int r2 = 4700; //Value of R1 (ohms) in level shift voltage divider
  const float dueRefVoltage = 3.3; //Ref. https://www.arduino.cc/reference/en/language/functions/analog-io/analogreference/ 
  const int dueNumSamples = 1024; //Ref. https://www.arduino.cc/reference/en/language/functions/analog-io/analogread/ 
  const float vioutq = 2.22; // VIout(Q) value of ACS712 sensor. The zero current output voltage.
                             // NB. vioutq(observed) = 2.22V; vioutq(calculated) = 1.65 
  const float sensitivity = 0.185; // ACS712 sensitivity in mV/A 
//Variables
  int sample; //The sample value between 0-1023
  float sampleVoltage; // The voltage (at the Arduino) represented by the sample value
  float sensorVoltage; // The calculated voltage at the sensor output (VIout)
  float readingAmps; // The calculated current reading (Ip)

void setup() {
  Serial.begin(115200); // Initialize the serial port
  // Set up the select pins as outputs:
  for (int i=0; i<3; i++) {
    pinMode(selectPins[i], OUTPUT);
    digitalWrite(selectPins[i], HIGH);
  }
  pinMode(zInput, INPUT); // Set up Z as an input
  Serial.println("**********************");
}

void loop() {
  selectMuxPin(pin); // Select which sensor to be read,
  sample = analogRead(zInput); // and read it as an analog value 0-1023
  sampleVoltage = float(sample) * dueRefVoltage / float(dueNumSamples); // Convert the sample value to a voltage 
  sensorVoltage = sampleVoltage * (r1+r2) / r2; //Calculate the sensor output voltage prior to the voltage divider
  readingAmps = (vioutq - sensorVoltage) / sensitivity;
  Serial.println(String(pin) + ":" + String(readingAmps));
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
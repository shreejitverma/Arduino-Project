#include <EEPROM.h>
#include <LiquidCrystal.h>


LiquidCrystal lcd(7, 5, 8, 9, 10, 11, 12);

byte Dispnum[10][2][3] = {

{ {255, 0, 255}, {255, 1, 255} },        // data to display "0"
{ {0, 255, 254}, {1, 255, 1} },          // data to display "1"
{ {2, 2, 255}, {255, 1, 1} },            // data to display "2"
{ {0, 2, 255}, {1, 1, 255} },            // data to display "3"
{ {255, 1, 255}, {254, 254, 255} },      // data to display "4"
{ {255, 2, 2}, {1, 1, 255} },            // data to display "5"
{ {255, 2, 2}, {255, 1, 255} },          // data to display "6"
{ {0, 0, 255}, {254, 255, 254} },        // data to display "7"
{ {255, 2, 255}, {255, 1, 255} },        // data to display "8"
{ {255, 2, 255}, {254, 254, 255} }       // data to display "9"
};

int Distance;

int Value;






const byte SqWavePin = 4;
const byte Button1 = 6;                   // Pushbutton switch D6, connected to ground
const byte Button2 = 14;                  // Pushbutton switch A0, connected to ground
const byte Button3 = 15;                  // Pushbutton switch A1, connected to ground
const byte Button4 = 16;                  // Pushbutton switch A2, connected to ground

byte MenuMode = 0;                        // MenuMode 0 means not in a menu

boolean MenuModeActive = false;           // Only goes true when a menu item has been selected

unsigned long ButtonsOffTime;             // How long since last button was released

boolean Button1State;                     // State of Button1 from last cycle
boolean Button2State;                     // State of Button2 from last cycle
boolean Button3State;                     // State of Button3 from last cycle
boolean Button4State;                     // State of Button4 from last cycle
boolean ButtonPressed = false;            // Becomes true when a button press is detected
boolean ButtonPressedPrev = false;        // Previous value of ButtonPressed

int DistancePrev;                          // Previous value of Distance during calibration

int SensorCount = 0;                      // Speed Sensor pulse count
int TimerCount = 0;                       // Timer2 overflow count
int Freq;                                 // Copy of SensorCount, and is the actual vehicle speed
boolean Trigger = false;                  // When it becomes true it triggers the print cycle
boolean SqWaveOut = false;                // Used to toggle output state of pin 4
int FreqPrev1 = 0;                        // Previous value of Freq, used for averaging speed
int FreqPrev2 = 0;                        // Another previous value of Freq, used for averaging speed
int Speed;                                // Average of all 3 Freq variables, and is what is displayed on LCD

int DistanceMiles;                        // Cumulative distance travelled in miles
int DistanceTenths;                       // Cumulative distance travelled, tenths digit
int DistanceClicks;                       // Cumulative speed sensor pulses received, reduced by Distance each 1/10 mile
unsigned long TotalMiles;                 // Total distance travelled in miles
int TotalTenths;                          // Total distance travelled, tenths digit
int TotalClicks;                          // Total speed sensor pulses received, reduced by Distance each 1/10 mile

long DistanceFeet;                        // Cumulative distance travelled in feet, temporary display option
unsigned int DistanceFeetTenths;          // Cumulative distance travelled in feet, tenths digit, temporary display option
int FeetScaleFactor;                      // Scale Factor used to calculate feet from speed sensor pulses


ISR(TIMER2_OVF_vect) {                    // This is the Interrupt Service Routine, called when Timer2 overflows

 TimerCount +=8;                         // Increment TimerCount by 8
 if (TimerCount > Value)  {           // Check to see if it's time to display the count
   Freq = SensorCount;                   // Copies the interrupt 0 count into variable "Freq"
   Trigger = true;                       // sets "Trigger" to let the main loop know it's time to print data
   SensorCount = 0;                      // Reset the interrupt 0 count
   TimerCount -= Value;               // Reset Timer2 count, but keep the excess for next time around
 }
 SqWaveOut = !SqWaveOut;                 // Toggle SqWaveOut to its opposite state
 digitalWrite(SqWavePin, SqWaveOut);     // Set pin 4 to the new state of SqWaveOut

}


void setup()  {

// turn on DigitalPin 13 LED to signal that the custom characters have been loaded into the LCD
 pinMode(13, OUTPUT);
 loadchars();
 digitalWrite(13, HIGH);

 pinMode(SqWavePin, OUTPUT);                  // Output on pin 4 will be oscillating
 pinMode(Button1, INPUT);                     // Set Button1 pin as input
 pinMode(Button2, INPUT);                     // Set Button2 pin as input
 pinMode(Button3, INPUT);                     // Set Button3 pin as input
 pinMode(Button4, INPUT);                     // Set Button4 pin as input
 digitalWrite(Button1, HIGH);                 // Turn on Button1's internal pull-up resistor
 digitalWrite(Button2, HIGH);                 // Turn on Button2's internal pull-up resistor
 digitalWrite(Button3, HIGH);                 // Turn on Button3's internal pull-up resistor
 digitalWrite(Button4, HIGH);                 // Turn on Button4's internal pull-up resistor

 Distance = (int(EEPROM.read(101)) * 256) + (int(EEPROM.read(102)));  // read Distance out of EEPROM
 Value = int((2812500.0 / Distance) + .5);               // calculate Value from Distance

 // set up the LCD's number of columns and rows:
 lcd.begin(16,2);

 // read total mileage out of EEPROM
 TotalMiles = (EEPROM.read(5) * 65536) + (EEPROM.read(6) * 256) + (EEPROM.read(7));
 TotalTenths = EEPROM.read(8);
 TotalClicks = (EEPROM.read(9) * 256) + (EEPROM.read(10));

 // read Distance out of EEPROM
 DistanceMiles = (EEPROM.read(0) * 256) + (EEPROM.read(1));
 DistanceTenths = EEPROM.read(2);
 DistanceClicks = (EEPROM.read(3) * 256) + (EEPROM.read(4));

// print the speed in big font
   lcd.setCursor(0, 0);                  // set the cursor to (13,0)
   Freq = Freq % 1000;                   // drop any digits above 999
   printbigchar(int(Freq / 100),0);      // print the speed hundreds
   Freq = Freq % 100;                    // drop any digits above 99
   printbigchar(int(Freq / 10),1);       // print the speed tens
   Freq = Freq % 10;                     // drop any digits above 9
   printbigchar(Freq,2);                 // print the speed ones
 // write Distance to LCD
 printmileage();

 attachInterrupt(0, AddSensorCount, RISING);  // Interrupt 0 is on digital pin 2
 attachInterrupt(1, EEPROMwrite, FALLING);    // Interrupt 1 is on digital pin 3

 //Timer2 Settings: Timer Prescaler / 64, mode 0
 //Timmer clock = 16 MHz / 64 = 250 KHz or 0.5us
 TCCR2A = 0;
 TCCR2B = 1<<CS22 | 0<<CS21 | 0<<CS20;      // Set Timer2 frequency to 250 KHz
                                            // Used to be 010 for 2 MHz clock

 //Timer2 Overflow Interrupt Enable
 TIMSK2 = 1<<TOIE2;




 ButtonsOffTime = millis();                 // Set ButtonStateTime to current time

}



void loop()  {

 if (Trigger)  {                         // Print data if the "Trigger" is set
// calculate average speed for printing
   Speed = int((Freq + FreqPrev1 + FreqPrev2) / 3);

// print the speed in big font
   Speed = Speed % 1000;                 // drop any digits above 999
   printbigchar(int(Speed / 100), 0);    // print the speed hundreds
   Speed = Speed % 100;                  // drop any digits above 99
   printbigchar(int(Speed / 10), 1);     // print the speed tens
   Speed = Speed % 10;                   // drop any digits above 9
   printbigchar(Speed, 2);               // print the speed ones

// Main odometer update
   TotalClicks += Freq;                  // Add Freq to TotalClicks
   while (TotalClicks >= Distance)  {     // Check if TotalClicks has reached Distance (1/10 mile travelled)
     TotalTenths ++;                     // increment TotalTenths and
     TotalClicks -=Distance;              // reduce TotalClicks by Distance
   }
   while (TotalTenths >= 10)  {          // Check if TotalTenths has reached 10
     TotalMiles ++;                      // if so, increment TotalMiles and
     TotalTenths -= 10;                  // decrease TotalTenths by 10
   }
   while (TotalMiles > 99999)  {         // Check if TotalMiles has reached 100000
     TotalMiles -= 100000;               // if so, decrease TotalMiles by 100000 (rollover odometer)
   }

// Trip odometer update
   DistanceClicks += Freq;               // Add Freq to DistanceClicks
   while (DistanceClicks >= Distance)  {  // Check if DistanceClicks has reached Distance (1/10 mile travelled)
     DistanceTenths ++;                  // if so, increment DistanceTenths and
     DistanceClicks -=Distance;           // reduce DistanceClicks by Distance
   }
   while (DistanceTenths >= 10)  {       // Check if DistanceTenths has reached 10
     DistanceMiles ++;                   // if so, increment DistanceMiles and
     DistanceTenths -= 10;               // decrease DistanceTenths by 10
   }
   while (DistanceMiles > 999)  {        // Check if DistanceMiles has reached 1000
     DistanceMiles -= 1000;              // if so, decrease DistanceMiles by 1000 (rollover odometer)
   }

   if (MenuMode == 0)  {                 // if we're not in a menu
     printmileage();                     // then print the odometer
   }

// foot distance display mode
// DistanceFeetTenths is value is 100 times the actual value. This to allow for
// much greater precision while avoiding the use of a float variable.
   if ((MenuMode == 2) && MenuModeActive)  {          // are we in foot distance display mode? If so...
     DistanceFeetTenths += (Freq * FeetScaleFactor);  // Update DistanceFeetTenths
     while (DistanceFeetTenths >= 1000)  {            // Has the tenths digit overflowed (exceeded 10.00)?
       DistanceFeet ++;                               // If so, increment DistanceFeet by 1,
       DistanceFeetTenths -= 1000;                    // and decrease tenths digit by 1000 (represending 10.00)
     }
     while (DistanceFeet > 99999)  {                  // Has DistanceFeet reached 100000?
       DistanceFeet -= 100000;                        // If so, roll it over (drop it by 100000 feet)
     }
     printfeet();                                     // print the distance in feet
   }

// copy speed values for use next cycle
   FreqPrev2 = FreqPrev1;
   FreqPrev1 = Freq;

   Trigger = false;                      // Reset "Trigger"...until TimerCount reaches Value again
 }

// Read the pushbutton switches for user input
 Button1State = digitalRead(Button1);    // Read the state of Button1
 Button2State = digitalRead(Button2);    // Read the state of Button2
 Button3State = digitalRead(Button3);    // Read the state of Button3
 Button4State = digitalRead(Button4);    // Read the state of Button4

 // ButtonPressed is true if any buttons are pressed, otherwise it's false
 ButtonPressed = !(Button1State && Button2State && Button3State && Button4State);

 // If a button was just pressed for the first time then reset the ButtonsOffTime timer
 if ((ButtonPressed == false) && (ButtonPressedPrev == true))  {
   ButtonsOffTime = millis();
 }




 // test to see if it's been at over 50 mS since last button was released
  // and that a button is being pressed for the first time around
  if (((millis() - ButtonsOffTime) > 50) && (ButtonPressed == true) && (ButtonPressedPrev == false))  {

    switch (MenuMode)  {                              // which menu is currently active?

      case 1:  {                                      // MenuMode 1 is the Calibration screen

        if (Button1State == LOW) {                    // test for Button1 being pressed
          if (MenuModeActive)  {                      // test to see if a menu mode has been selected
            MenuModeActive = false;                   // Set MenuModeActive to false
            lcd.setCursor(13, 0);                     // set the cursor to (13,0)
            lcd.print("  CAL   ");                    // overwrite total mileage with "CAL" & spaces
            lcd.setCursor(13, 1);                     // set the cursor to (13,1)
            lcd.print("       ");                     // overwrite Distance with spaces
            //write Distance to EEPROM:
            EEPROM.write(101, byte((Distance & 65280)/256));
            EEPROM.write(102, byte((Distance & 255)));
          }
          else  {
            MenuModeActive = true;                    // Set MenuModeActive to true
            DistancePrev = Distance;                    // copy Distance to DistancePrev
            lcd.setCursor(13, 0);                     // set the cursor to (13,0)
            lcd.print("  CAL:  ");                    // overwrite total mileage with "CAL:" & spaces
            lcd.setCursor(13, 1);                     // set the cursor to (13,1)
            lcd.print("       ");                     // overwrite trip mileage with spaces
            lcd.setCursor(15, 1);                     // set the cursor to (13,1)
            lcd.print(Distance);                       // print Distance
          }
        }

        if (Button2State == LOW)  {                   // test for Button2 being pressed
          if (MenuModeActive)  {                      // test to see if a menu mode has been selected
            MenuModeActive = false;                   // Set MenuModeActive to false
            lcd.setCursor(13, 0);                     // set the cursor to (13,0)
            lcd.print("  CAL   ");                    // overwrite total mileage with "CAL" & spaces
            lcd.setCursor(13, 1);                     // set the cursor to (13,1)
            lcd.print("       ");                     // overwrite Distance with spaces
            Distance = DistancePrev;                    // set Distance back to DistancePrev (toss changes)
            Value = int((2812500.0 / Distance) + .5); // recalculate Value from Distance
          }
          else  {
            MenuMode = 0;                             // Set MenuMode to 0
            lcd.setCursor(13, 0);                     // set the cursor to (13,0)
            lcd.print("       ");                     // overwrite "CAL" with spaces
            lcd.setCursor(13, 1);                     // set the cursor to (13,1)
            lcd.print("       ");                     // overwrite Distance with spaces
            printmileage();                           // display the odometer
          }
        }

        if (Button3State == LOW)  {                   // test for Button3 being pressed
          if (MenuModeActive)  {                      // test to see if a menu mode has been selected
            Distance ++;                               // Increment Distance
            Value = int((2812500.0 / Distance) + .5); // recalculate Value from Distance
            lcd.setCursor(15, 1);                     // set the cursor to (13,0)
            lcd.print(Distance);                       // print Distance
          }
          else  {
            MenuMode = 2;                             // Go to the previous menu
            lcd.setCursor(13, 0);                     // set the cursor to (13,0)
            lcd.print(" FEET  ");                     // overwrite menu item with "FEET:" & spaces
            lcd.setCursor(13, 1);                     // set the cursor to (13,1)
            lcd.print("       ");                     // overwrite lower menu area with spaces
          }
        }

        if (Button4State == LOW)  {                   // test for Button4 being pressed
          if (MenuModeActive)  {                      // test to see if a menu mode has been selected
            Distance --;                               // decrement Distance
            Value = int((2812500.0 / Distance) + .5); // recalculate Value from Distance
            lcd.setCursor(15, 1);                     // set the cursor to (13,0)
            lcd.print(Distance);                       // print Distance
          }
          else  {
            MenuMode = 2;                             // Go to the previous menu
            lcd.setCursor(13, 0);                     // set the cursor to (13,0)
            lcd.print(" FEET  ");                     // overwrite menu item with "FEET:" & spaces
            lcd.setCursor(13, 1);                     // set the cursor to (13,1)
            lcd.print("       ");                     // overwrite lower menu area with spaces
          }
        }

        break;
      }

      case 2:  {                                      // MenuMode 2 is the Foot distance meter

        if (Button1State == LOW)  {                   // test for Button1 being pressed
          if (MenuModeActive)  {                      // test to see if a menu mode has been selected
            DistanceFeet = 0;                         // Reset DistanceFeet to 0
            DistanceFeetTenths = 0;                   // Reset DistanceFeetTenths to 0
            printfeet();                              // Print the distance in feet
          }
          else  {
            MenuModeActive = true;                    // Set MenuModeActive to true (enter this mode)
            lcd.setCursor(13, 0);                     // set the cursor to (13,0)
            lcd.print(" FEET: ");                     // overwrite total mileage with "FEET:" & spaces
            DistanceFeet = 0;                         // Reset DistanceFeet to 0
            DistanceFeetTenths = 0;                   // Reset DistanceFeetTenths to 0
            FeetScaleFactor = 528000 / Distance;       // Calculate FeetScaleFactor from Value
            printfeet();                              // Print the distance in feet
          }
        }

        if (Button2State == LOW)  {                   // test for Button2 being pressed
          if (MenuModeActive)  {                      // test to see if a menu mode has been selected
            MenuModeActive = false;                   // Set MenuModeActive to true (exit this mode)
            lcd.setCursor(13, 0);                     // set the cursor to (13,0)
            lcd.print(" FEET  ");                     // overwrite total mileage with "FEET:" & spaces
            lcd.setCursor(13, 1);                     // set the cursor to (13,1)
            lcd.print("       ");                     // overwrite feet distance with spaces
          }
          else  {
            MenuMode = 0;                             // Set MenuMode to 0
            lcd.setCursor(13, 0);                     // set the cursor to (13,0)
            lcd.print("       ");                     // overwrite "CAL" with spaces
            lcd.setCursor(13, 1);                     // set the cursor to (13,1)
            lcd.print("       ");                     // overwrite Distance with spaces
            printmileage();                           // display the odometer
          }
        }

        if (Button3State == LOW)  {                   // test for Button3 being pressed
          if (MenuModeActive)  {                      // test to see if a menu mode has been selected
            // do nothing
          }
          else  {
            MenuMode = 1;                             // Go to the previous menu
            lcd.setCursor(13, 0);                     // set the cursor to (13,0)
            lcd.print("  CAL  ");                     // overwrite menu item with "CAL" & spaces
            lcd.setCursor(13, 1);                     // set the cursor to (13,1)
            lcd.print("       ");                     // overwrite lower menu area with spaces
          }
        }

        if (Button4State == LOW)  {                   // test for Button4 being pressed
          if (MenuModeActive)  {                      // test to see if a menu mode has been selected
            // do nothing
          }
          else  {
            MenuMode = 1;                             // Go to the previous menu
            lcd.setCursor(13, 0);                     // set the cursor to (13,0)
            lcd.print("  CAL  ");                     // overwrite menu item with "CAL" & spaces
            lcd.setCursor(13, 1);                     // set the cursor to (13,1)
            lcd.print("       ");                     // overwrite lower menu area with spaces
          }
        }

        break;
      }






 default:  {                                   // MenuMode 0 is the default screen
     if ((Button1State) == LOW)  {               // test for Button1 being pressed
       DistanceMiles = 0;                        // if so, reset distance miles to zero
       DistanceTenths = 0;                       // also, reset the tenths to zero
       DistanceClicks = 0;                       // finally, reset the clicks to zero
       printmileage();                           // display the odometer
       break;
     }

     if ((Button2State) == LOW)  {               // test for Button2 being pressed
       MenuMode = 1;                             // Set MenuMode to 1
       DistancePrev = Distance;                    // copy Distance to DistancePrev
       lcd.setCursor(13, 0);                     // set the cursor to (13,0)
       lcd.print("  CAL  ");                     // overwrite total mileage with "CAL" & spaces
       lcd.setCursor(13, 1);                     // set the cursor to (13,1)
       lcd.print("       ");                     // overwrite trip mileage with spaces
       break;
     }

   }
 }
}

ButtonPressedPrev = ButtonPressed;        // store ButtonPressed in ButtonPressedPrev for test on next loop

}


void AddSensorCount()  {                  // This is the subroutine that is called when interrupt 0 goes high
SensorCount++;                          // Increment SensorCount by 1
}


void EEPROMwrite()  {                     // This is the subroutine that is called when interrupt 1 goes low
// store mileage variables in EEPROM
 EEPROM.write(0, byte((DistanceMiles & 65280)/256));
 EEPROM.write(1, byte((DistanceMiles & 255)));
 EEPROM.write(2, byte(DistanceTenths));
 EEPROM.write(3, byte((DistanceClicks) & 65280)/256);
 EEPROM.write(4, byte((DistanceClicks) & 255));
 EEPROM.write(5, byte((TotalMiles & 16711680)/65536));
 EEPROM.write(6, byte((TotalMiles & 65280)/256));
 EEPROM.write(7, byte((TotalMiles & 255)));
 EEPROM.write(8, byte(TotalTenths));
 EEPROM.write(9, byte((TotalClicks) & 65280)/256);
 EEPROM.write(10, byte((TotalClicks) & 255));
}


void loadchars() {                        // This subroutine programs the custom character data into the LCD
lcd.command(64);
// Custom character 0
lcd.write(byte(B11111));
lcd.write(byte(B11111));
lcd.write(byte(B11111));
lcd.write(byte(B00000));
lcd.write(byte(B00000));
lcd.write(byte(B00000));
lcd.write(byte(B00000));
lcd.write(byte(B00000));

// Custom character 1
lcd.write(byte(B00000));
lcd.write(byte(B00000));
lcd.write(byte(B00000));
lcd.write(byte(B00000));
lcd.write(byte(B00000));
lcd.write(byte(B11111));
lcd.write(byte(B11111));
lcd.write(byte(B11111));

// Custom character 2
lcd.write(byte(B11111));
lcd.write(byte(B11111));
lcd.write(byte(B11111));
lcd.write(byte(B00000));
lcd.write(byte(B00000));
lcd.write(byte(B11111));
lcd.write(byte(B11111));
lcd.write(byte(B11111));

// Custom character 3
lcd.write(byte(B00000));
lcd.write(byte(B00000));
lcd.write(byte(B00000));
lcd.write(byte(B00000));
lcd.write(byte(B00000));
lcd.write(byte(B01110));
lcd.write(byte(B01110));
lcd.write(byte(B01110));

// Custom character 4
lcd.write(byte(B00000));
lcd.write(byte(B00000));
lcd.write(byte(B00000));
lcd.write(byte(B01110));
lcd.write(byte(B01110));
lcd.write(byte(B01110));
lcd.write(byte(B00000));
lcd.write(byte(B00000));

// Custom character 5
lcd.write(byte(B00000));
lcd.write(byte(B00000));
lcd.write(byte(B00000));
lcd.write(byte(B00000));
lcd.write(byte(B00000));
lcd.write(byte(B00000));
lcd.write(byte(B00000));
lcd.write(byte(B00000));

// Custom character 6
lcd.write(byte(B00000));
lcd.write(byte(B00000));
lcd.write(byte(B00000));
lcd.write(byte(B00000));
lcd.write(byte(B00000));
lcd.write(byte(B00000));
lcd.write(byte(B00000));
lcd.write(byte(B00000));

// Custom character 7
lcd.write(byte(B00000));
lcd.write(byte(B00000));
lcd.write(byte(B00000));
lcd.write(byte(B00000));
lcd.write(byte(B00000));
lcd.write(byte(B00000));
lcd.write(byte(B00000));
lcd.write(byte(B00000));

lcd.home();
}


void printmileage()  {
lcd.setCursor(13, 0);                               // set the cursor to (13,0)
lcd.print(int((TotalMiles % 100000) / 10000));      // print the total miles ten-thousands digit
lcd.print(int((TotalMiles % 10000) / 1000));        // print the total miles thousands digit
lcd.print(int((TotalMiles % 1000) / 100));          // print the total miles hundreds digit
lcd.print(int((TotalMiles % 100) / 10));            // print the total miles tens digit
lcd.print(TotalMiles % 10);                         // print the total miles ones digit
lcd.print(".");                                     // print the decimal point
lcd.print(TotalTenths);                             // print the total miles tenths digit
lcd.setCursor(14, 1);                               // set the cursor to (15,0)
lcd.print(int((DistanceMiles % 1000) / 100));       // print the distance miles hundreds digit
lcd.print(int((DistanceMiles % 100) / 10));         // print the distance miles hundreds digit
lcd.print(DistanceMiles % 10);                      // print the distance miles hundreds digit
lcd.print(".");                                     // print the decimal point
lcd.print(DistanceTenths);                          // print the distance miles tenths digit
}


void printfeet()  {
lcd.setCursor(13, 0);                               // set the cursor to (13,0)
lcd.print(" FEET: ");                               // print the FEET units
lcd.setCursor(13, 1);                               // set the cursor to (13,0)
lcd.print(int((DistanceFeet % 100000) / 10000));    // print the distance feet ten thousands digit
lcd.print(int((DistanceFeet % 10000) / 1000));      // print the distance feet thousands digit
lcd.print(int((DistanceFeet % 1000) / 100));        // print the distance feet hundreds digit
lcd.print(int((DistanceFeet % 100) / 10));          // print the distance feet tens digit
lcd.print(DistanceFeet % 10);                       // print the distance feet ones digit
lcd.print(".");                                     // print the decimal point
lcd.print(DistanceFeetTenths);                      // print the distance feet tenths digit
}


void printbigchar(byte digit, byte col) {
if (digit > 9) return;
for (int i = 0; i < 2; i++) {
lcd.setCursor(col*4 , i);
for (int j = 0; j < 3; j++) {
  lcd.write(Dispnum[digit][i][j]);
}
lcd.write(254);
}

lcd.setCursor(col + 4, 0);
}

#include <FastLED.h>
#include "RTClib.h"
#include <Wire.h>

#define LED_PIN 6 // Pin for the led matrix data output
#define LED_NUM 64 // Number of leds in the matrix
#define BD_NUM 24 // Number of total bit dots needed for all displays.
#define PR A0 // Photoresitor data in
#define BUTT_1 9 // Button 1
#define BUTT_2 7 // Button 2
#define BUTT_3 8 // Button 3
#define MODE_LIM 2 // Limit of display modes.
#define BACKG_NUM 255 // Number of backgrounds  
#define ACCEL_PORT 0x69 // Wire address of the accelerometer
#define TIP_POINT 20000 // At what point the dots ball out of place.
#define FADE_RATE 2 // Rate at which dots fade colors (has to be a multiple of 2)
#define RESET 500 // Time before the clock resest the dots
#define DOT_MOVE 700 // The resistence for dot movment
#define B_HIGH 200 // Brightness high limit
#define B_LOW 2 // Brightness low limit
#define BLANK CRGB(0,0,0) // A blank CRGB object.
#define SIX_BIT 6 // Literally the number six.
#define FOUR_BIT 4 // Literally the number 4
#define BYTE 8 // Literally the number 8
#define DIS_NUM 2 // The number of displays
#define BG_BRIGHT 100 // Background brightness
#define BG_CHANGE 20 // How far the background jumps in the background palette
#define BG_SPEED 50 // The rate at which you can move through backgrounds
#define PAL_NUM 9 // The number of palettes.


DEFINE_GRADIENT_PALETTE(MAP_WHITEBLUE) {
    0, 0, 0, 230,
    128, 0, 100, 155,
    255, 0, 0, 230
};

DEFINE_GRADIENT_PALETTE(MAP_WHITEGREEN) {
    0, 0, 230, 0,
    128, 75, 150, 25,
    255, 0, 230, 0,
};

DEFINE_GRADIENT_PALETTE(MAP_ORGRED) {
    0, 230, 0, 0,
    128, 150, 100, 0,
    255, 230, 0, 0
};

DEFINE_GRADIENT_PALETTE(MAP_YLWWHITE) {
    0, 75, 75, 75,
    128, 150, 100, 0,
    255, 75, 75, 75
};

DEFINE_GRADIENT_PALETTE(MAP_FIRE) {
    0, 150, 100, 0,
    33, 200, 50, 0,
    66, 100, 150, 0,
    99, 50, 150, 0,
    132, 50, 200, 0,
    185, 100, 150, 0,
    208, 200, 50, 0,
    255, 150, 100, 0

};

DEFINE_GRADIENT_PALETTE(MAP_BLUEGREENWHT) {
  0, 20, 20, 200,
  64, 0, 230, 0,
  128, 75, 75, 75,
  196, 0, 230, 0,
  255, 20, 20, 200
};

DEFINE_GRADIENT_PALETTE(MAP_PINK) {
  0, 150, 0, 100,
  128, 50, 0, 200,
  255, 150, 0, 100,
};

DEFINE_GRADIENT_PALETTE(MAP_GRNPRPLE) {
  0, 200, 0, 50,
  128, 0, 230, 0,
  255, 200, 0, 50,
};

const TProgmemRGBGradientPalette_byte* PaletteArr[] = {
MAP_YLWWHITE,
MAP_WHITEGREEN,
MAP_ORGRED,
MAP_WHITEBLUE,
MAP_FIRE,
Rainbow_gp,
MAP_BLUEGREENWHT,
MAP_PINK,
MAP_GRNPRPLE,
};




/*
* Takes a PhotoResistor to read a returns nice smooth values back.
* You can use setLimits to adjust what values you are reciveing.
*/
class PhotoResistorSmoother{ 
  
    public: // Give it the number of the port, along the high and low limits of the values you want back. 
      PhotoResistorSmoother(uint8_t portNum) {
        port = portNum;
      }

      // returns the smoothed out value of the PhotoResistor.
      int getValue() {
        target = analogRead(port);
        value += floor((target - value) / 8);
        return map(value, 7, 1016, lowRead, highRead);
      }

      void setLimits(int32_t low, int32_t high) { // Resets the limits on the photoresistor reader.
        highRead = high;
        lowRead = low;
      }

    private:
      uint8_t port = 0;
      int32_t target = 0;
      int32_t value = 0;
      int32_t highRead = 1023;
      int32_t lowRead = 0;
};


//
 /**
 * The button class takes in a port number to read.
 * Call the check function in your loop to check if the button was pressed.
 * use the result to decide what to do based on how long the button was held down for.
 */

class GY521Reader {

  public:
    void begin() {
      Wire.beginTransmission(Port); // Begins a transmission to the I2C slave (GY-521 board)
      Wire.write(0x6B); // PWR_MGMT_1 register
      Wire.write(0); // set to zero (wakes up the MPU-6050)
      Wire.endTransmission(true);
    }

    void updateReadings() {
      Wire.beginTransmission(Port);
      Wire.write(0x3B);
      Wire.endTransmission(false); 
      Wire.requestFrom(Port, 6, true); 
      // "Wire.read()<<8 | Wire.read();" means two registers are read and stored in the same variable
      XReading = Wire.read()<<8 | Wire.read(); // reading registers: 0x3B (ACCEL_XOUT_H) and 0x3C (ACCEL_XOUT_L)
      YReading = Wire.read()<<8 | Wire.read(); // reading registers: 0x3D (ACCEL_YOUT_H) and 0x3E (ACCEL_YOUT_L)
      ZReading = Wire.read()<<8 | Wire.read(); // reading registers: 0x3F (ACCEL_ZOUT_H) and 0x40 (ACCEL_ZOUT_L)
    }

    // Adjust the return types for different accelerometer orintations.
    int16_t X() { return YReading / adjuster; }
    int16_t Y() { return XReading / adjuster; }
    int16_t Z() { return ZReading / adjuster; }

  private:
    uint8_t Port = ACCEL_PORT;
    const int16_t adjuster = 60; 
    int16_t XReading;
    int16_t YReading;
    int16_t ZReading;
};

 /**
 * The button class takes in a port number to read.
 * Call the check function in your loop to check if the button was pressed.
 * use the result to decide what to do based on how long the button was held down for.
 */

class Button{

    public:
        Button(uint8_t port) {
            Port = port;
            pinMode(Port, INPUT_PULLUP);
        }

        bool isPressed() {
          if (digitalRead(Port) == LOW) {
            Count++;
            return true;
          } else {
            return false;
          }
        }

        void clearCount() {
          Count = 0;
        }
        
        uint32_t getCount() {
            return Count;
        }

    private:
        uint32_t Count = 0; 
        uint8_t Port = 0;
};

enum class Action{ ModeChange, TimeAdjust, DoNothing, PaletteChange};

/**
* The control board has control over all the external controls on the clock.
* This includes the three buttons along with the potenitometer reader.
* The function goIntoAdjustMode is meant to be inside a loop.
* It keeps an eye out for the main button being held down long enough to get tossed into
* adjust mode.
* In this mode it has two function that return either a 1 or -1 to adjust hours and minutes.
* this depends on how long the adjuster buttons have been held down for.
* It will also give out the potentiometer readings. 
*/
class ControlBoard{
    public:
        ControlBoard(uint8_t pin1, uint8_t pin2, uint8_t pin3, uint8_t pin4) : MainButt(pin1), HourButt(pin2), MinButt(pin3), PR_Reader(pin4) {
          PR_Reader.setLimits(B_LOW, B_HIGH);
        }

        int8_t getMode() {
            return Mode;
        }

        int8_t getBGIndex() {
            return BGIndex;
        }

        int8_t getPalIndex() {
          return PalIndex;
        }
        
        Action buttonCheck() { // Returns true if it's time to go into time adjust mode
            if (adjustMode) {
              return Action::TimeAdjust;
            } else {
              int8_t tempMode = Mode;
              int8_t tempPal = PalIndex;
              mainButtCheck();
              hourButtCheck();
              minuteButtCheck();
              if (tempMode != Mode) {
                return Action::ModeChange;
              }
              if (tempPal != PalIndex) {
                return Action::ModeChange;
              }
            }
            return Action::DoNothing;
        }

        void stayInAdjust() {
          if (!MainButt.isPressed() && MainButt.getCount() > 1) {
            MainButt.clearCount();
            adjustMode = false;
          }
        }

        int8_t getHourUpdate() {
            if (!HourButt.isPressed() && HourButt.getCount() > 1) {
              uint32_t count = HourButt.getCount();
              HourButt.clearCount();
              if (HalfSecond > count) {return 1;}
              if (count > HalfSecond) {return -1;}
            }
            return 0;
        }

        int8_t getMinUpdate() {
          if (!MinButt.isPressed() && MinButt.getCount() > 1) {
            uint32_t count = MinButt.getCount();
            MinButt.clearCount();
            if (HalfSecond > count) {return 1;}
            if (count > HalfSecond) {return -1;}
          }
          return 0;
        }

        int8_t getPRReading() {
          return PR_Reader.getValue();
        }
        

    private:
        Button MainButt;
        Button HourButt;
        Button MinButt;
        PhotoResistorSmoother PR_Reader; 
        const uint16_t HalfSecond = 300;
        const uint16_t MultiSecond = 1500;
        int8_t Mode = 0;
        uint8_t BGIndex = random8();
        int8_t PalIndex = 0;
        bool adjustMode = false;
        void modeLimitCheck() {
            if (Mode >= MODE_LIM) {
                Mode = 0;
            } 
            if (0 > Mode) {
                Mode = MODE_LIM - 1;
            }
        }

        void palLimitCheck() {
          if (PalIndex >= PAL_NUM) {
            PalIndex = 0;
          }
          if (0 > PalIndex) {
            PalIndex = PAL_NUM - 1;
          }
        }

        void mainButtCheck() {
            if (!MainButt.isPressed() && MainButt.getCount() > 1) {
              uint32_t count = MainButt.getCount();
              MainButt.clearCount();
              if (MultiSecond > count) {Mode++; modeLimitCheck();}
              if (count > MultiSecond) {adjustMode = true;}
            }
        }

        void hourButtCheck() {
            if (!HourButt.isPressed() && HourButt.getCount() > 1) {
              uint32_t count = HourButt.getCount();
              HourButt.clearCount();
              if (HalfSecond > count) {BGIndex += BG_CHANGE;}
            } else if (HourButt.isPressed()) {
              uint32_t count = HourButt.getCount();
              if (count > HalfSecond) { EVERY_N_MILLIS(BG_SPEED) { BGIndex++; } }
            }
        }

        void minuteButtCheck() {
          if (!MinButt.isPressed() && MinButt.getCount() > 1) {
            uint32_t count = MinButt.getCount();
            MinButt.clearCount();
            if (HalfSecond > count) {PalIndex ++; palLimitCheck();}
          }
        }

};

/**
 * The bitdot class creates a led that will react to gravity along with chanign binary colors
 * Set the fixed location in the formation that you want the clock itself in 
 * Ask the dot to move and it will freely do so when given readings from an accelerometer.
 * To stop it from moving, stop asking it to and tell it to go back to its original locaton. 
 * 
 */

class BitDots{
    public:
        void begin() { 
            FastLED.addLeds<WS2812B, LED_PIN, GRB>(FLEDS, LED_NUM);
            FastLED.setBrightness(255);
        }

        void SHOW_ALL_DOTS() {
          FastLED.show();
        }

        void CLEAR_ALL_DOTS() {
          FastLED.clear();
        }

        void DISPLAY_BACKGROUND(CRGB &BackG) {
          for (int i = 0; i < 8; ++i) {
            for (int j = 0; j < 8; ++j) {
              if (!Locations[i][j]) {
                displayBackground(GetLineLocation(i, j), BackG);
              }
            }
          }
        }

        void ALL_DOT_BRIGHTNESS(uint8_t bright) {
          FastLED.setBrightness(bright);
        }

        void CLEAN_ALL_LOCATIONS() {
          for (int i = 0; i < 8; ++i) {
            for (int j = 0; j < 8; ++j) {
              Locations[i][j] = false;
            }
          }
        }

        void hardReset() {
          xFixed = -1;
          x = xFixed;
          yFixed = -1;
          y = yFixed;
          ColorIndex = 0;
          Fading = false;
          IsAZero = true;
        }

        void setFixedLocation(int8_t xSet, int8_t ySet) {
            xFixed = xSet;
            x = xFixed;
            yFixed = ySet;
            y = yFixed;
        }

        void setColorPalette(CRGBPalette16 *pal) {
            ColorPalette = pal;
        }

        void setToOne() {
            if (IsAZero) {
                Fading = true;
                IsAZero = false;
            }
        }

        void setToZero() {
            if (!IsAZero) {
                Fading = true;
                IsAZero = true;
            }
        }
 
        void moveDot(int16_t xRead, int16_t yRead) {
            updatePulls(xRead, yRead); // Update the dots pull values
            int8_t xTemp = shiftDot(xPull, x); // Creates a theoretical dot in the direction it wants to move. This accounts for the board but not other dots.
            int8_t yTemp = shiftDot(yPull, y); 
            if (!Locations[xTemp][yTemp]) { // This where the dot wants to move against the locations of all the other dots.
            Locations[x][y] = false;  // this dot is no longer at that location
            x = xTemp; // If that location was okay move the dot there.
            y = yTemp;
          }
        }

        bool isInClockFormation() {
            return x == xFixed && y == yFixed;
        }

        void setToClockPosition() {
            x = xFixed;
            y = yFixed;
        }

        uint8_t GetLineLocation(int8_t xTemp, int8_t yTemp) {
            return xTemp + (yTemp * 8);
        }

        uint8_t GetLineLocation() {
            return x + (y * 8);
        }

        void displayDot() {
            fadeIfNeeded();
            Locations[x][y] = true; // Put the dot locatio back with the clock formation
            FLEDS[GetLineLocation()] = ColorFromPalette(*ColorPalette, ColorIndex);
        }



    private:
        uint8_t BGFader;
        int8_t xFixed;
        int8_t yFixed;
        int8_t x;
        int8_t y;
        int16_t xPull;
        int16_t yPull;
        uint8_t ColorIndex;
        bool Fading = false;
        const int16_t pullLimit = DOT_MOVE;
        inline static bool Locations[8][8];
        CRGBPalette16* ColorPalette;
        inline static CRGB FLEDS[LED_NUM];
        bool IsAZero = true;
        int8_t boundCheck(int8_t temp, int8_t incr) { // Make sure the new location is on the board.
              temp += incr;
              if (temp < 0) {
                return 0;
              } else if (7 < temp) {
                return 7;
              }
              return temp;
            }

            int8_t shiftDot(int16_t &tempPull, int8_t temp) { // Measure a pull value and see if it is enough to move a dot in that direction
              if (tempPull < -pullLimit) {
                tempPull = 0;
                return boundCheck(temp, -1);
              } else if (pullLimit < tempPull) {
                tempPull = 0;
                return boundCheck(temp, 1);
              }
              return temp;
            }

            void updatePulls(int16_t xRead, int16_t yRead) {
              xPull += (xRead) / 4; // Increment how hard the dot is being pulled by the readings
              yPull += (yRead)/ 4;
            }

            void fadeIfNeeded() {
                if (Fading) {
                  ColorIndex += FADE_RATE;
                  if (ColorIndex == 0 && IsAZero) {
                    Fading = false;
                  } else if (ColorIndex == 128 && !IsAZero) {
                    Fading = false;
                  }
                }
            }

          void displayBackground(int8_t location, CRGB &BGColor) {
            FLEDS[location] = BGColor;
            //FLEDS[location] = ColorFromPalette(FireBackGround, location * 10);
          }



};

/**
 * The abstract parent class for all clock displays
 * Each clock display needs to be able to tell an array of bit dots what time to show.
 * Along with a build function to be called once that tells them what shape to be in for that display. 
 * 
 */

class ClockDisplay {
    friend class CompleteClock;
    public: 
        // This sets the fixed loctions and color paletts for the bitdots.
        virtual void buildClock()=0;
        
        // Updates who is a zero and who is a one. 
        virtual void updateTime(DateTime now)=0;

        // Lets you know how many bit dots it will need for the display.
        virtual uint8_t requestNumOfDots()=0;

        BitDots& getBitDot(int8_t index) {
          return BitDotPointer[index];
        }

        CRGBPalette16& getPalette() {
          return PalettePointer;
        }

        void buildBitDigitHorizontal(int8_t xStart, int8_t yStart, uint8_t index, uint8_t length) {
          for (int i = 0; i < length; ++i) {
            BitDotPointer[index + i].setFixedLocation(xStart, yStart - i);
          }
        }

        void buildByteDigitVertical(int8_t xStart, int8_t yStart, uint8_t index) {
          int8_t xTemp = xStart;
          for (int i = 0; i < uint8_t(BYTE); ++i) {
            if (i == uint8_t(FOUR_BIT)) {
              yStart--;
              xTemp = xStart;
            }
            BitDotPointer[index + i].setFixedLocation((xTemp), yStart);
            xTemp++;
          }
        }

        void displayTimeAlongDots(uint8_t index, uint8_t size, uint8_t digit) {
          for (int i = index; i < (index + size); ++i) {
            if (digit % 2 == 1) {
              BitDotPointer[i].setToOne();
            } else {
              BitDotPointer[i].setToZero();
            }
            digit /= 2;
          }
        }

        void setSecondsIndex(uint8_t secondsIndex) {SecondsIndex = secondsIndex;}
        void setMinutesIndex(uint8_t minutesIndex) {MinutesIndex = minutesIndex;}
        void setHoursIndex(uint8_t hoursIndex) {HoursIndex = hoursIndex;}
        void setDotsNeeded(uint8_t dotsNeeded) {DotsNeeded = dotsNeeded;}
        uint8_t getSecondsIndex() {return SecondsIndex;}
        uint8_t getMinutesIndex() {return MinutesIndex;}
        uint8_t getHoursIndex() {return HoursIndex;}
        uint8_t getDotsNeeded() {return DotsNeeded;}

    private:
      inline static BitDots* BitDotPointer;
      inline static CRGBPalette16 PalettePointer;
      uint8_t SecondsIndex;
      uint8_t MinutesIndex;
      uint8_t HoursIndex;
      uint8_t DotsNeeded;
};

/**
 * This displays time horizontally in 16 bits.
 * two 6 bit arrays handle hours and minutes with the remaining 4 showng hours.
 * It displays white for 0 and yellow for 1. 
 * 
 */
class SixteenBitClock:public ClockDisplay{
  public:
    void buildClock() {
      setSecondsIndex(0);
      setMinutesIndex(6);
      setHoursIndex(12);
      setDotsNeeded(16);
      buildBitDigitHorizontal(1, 6, getSecondsIndex(), uint8_t(SIX_BIT));
      buildBitDigitHorizontal(3, 6, getMinutesIndex(), uint8_t(SIX_BIT));
      buildBitDigitHorizontal(6, 5, getHoursIndex(), uint8_t(FOUR_BIT));
      for (int i = 0; i < getDotsNeeded(); ++i) {
        getBitDot(i).setColorPalette(&getPalette());
      }
    }

    void updateTime(DateTime now) {
      displayTimeAlongDots(getSecondsIndex(), uint8_t(SIX_BIT), now.second());
      displayTimeAlongDots(getMinutesIndex(), uint8_t(SIX_BIT), now.minute());
      displayTimeAlongDots(getHoursIndex(), uint8_t(FOUR_BIT), now.hour() % 12);
    }

    uint8_t requestNumOfDots() {
      return getDotsNeeded();
    }
};

/**
 * Displays 3 bytes in 2 x 4 grids. Each one has it's own unique color.  
 * 
 */

class ThreeByteClock:public ClockDisplay{
  public:
  // Sets all the fixed x y values and sets the colors for the 3 byte clock.
        void buildClock() {
          setSecondsIndex(0);
          setMinutesIndex(8);
          setHoursIndex(16);
          setDotsNeeded(24);
          buildByteDigitVertical(0, 7, getSecondsIndex());
          buildByteDigitVertical(2, 4, getMinutesIndex());
          buildByteDigitVertical(4, 1, getHoursIndex());
          for (int i = 0; i < getDotsNeeded(); ++i) {
            getBitDot(i).setColorPalette(&getPalette());
          }
        }
        void updateTime(DateTime now) { // This desides what color to display the dot. 
          displayTimeAlongDots(getSecondsIndex(), uint8_t(FOUR_BIT), now.second() % 10);
          displayTimeAlongDots(getSecondsIndex() + FOUR_BIT, uint8_t(FOUR_BIT), now.second() / 10);
          displayTimeAlongDots(getMinutesIndex(), uint8_t(FOUR_BIT), now.minute() % 10);
          displayTimeAlongDots(getMinutesIndex() + FOUR_BIT, uint8_t(FOUR_BIT), now.minute() / 10);
          displayTimeAlongDots(getHoursIndex(), uint8_t(FOUR_BIT), now.hour() % 10);
          displayTimeAlongDots(getHoursIndex() + FOUR_BIT, uint8_t(FOUR_BIT), now.hour() / 10);
        }

        uint8_t requestNumOfDots() {
          return getDotsNeeded();
        }

};

/**
 * This runs the whole show. Takes readings from all the sensors and adjust displays accordingly.
 * It holds the bit dot array and gives it to the clock displays to adjust. 
 * 
 */

class CompleteClock{
    public:
        CompleteClock(
            uint8_t pin1,
            uint8_t pin2, 
            uint8_t pin3,
            uint8_t pin4
            ) :
            Controller(pin1, pin2, pin3, pin4) {}

        void begin() {
          Accelerometer.begin();
          BitDotArr[0].begin();
          RTC.begin();
          ClockDisplays[0] = &BitClock;
          ClockDisplays[1] = &ByteClock;
          MasterPal = MAP_YLWWHITE;
          ClockDisplay::BitDotPointer = BitDotArr;
          ClockDisplay::PalettePointer = MasterPal;
          ClockDisplays[0]->buildClock();
        }

        void runClock() {
          readInputs();
          manageController();
          manageDisplayMode();
          manageGravityMode(); 
          for (int i = 0; i < DotsInUse; ++i) {
            if (GravityMode) {
              BitDotArr[i].moveDot(x, y);
            }
            BitDotArr[i].displayDot();
          }
          refreshDots();
          delay(1);
        }

    private:
        BitDots BitDotArr[BD_NUM];
        RTC_DS1307 RTC;
        ThreeByteClock ByteClock;
        SixteenBitClock BitClock;
        ClockDisplay* ClockDisplays[DIS_NUM];
        ControlBoard Controller;
        GY521Reader Accelerometer;
        CRGBPalette16 MasterPal;
        CRGBPalette16 BackGroundPal = Rainbow_gp;
        bool GravityMode = false;
        int16_t x;
        int16_t y;
        int16_t z;
        uint8_t DotsInUse = BD_NUM;
        uint16_t tippingPoint = TIP_POINT;
        uint32_t GravityModeTimer = 0;
        uint32_t TimeLimit = RESET;
        

        void readInputs() {
          Accelerometer.updateReadings();
          x = Accelerometer.X();
          y = Accelerometer.Y() * -1;
          z = Accelerometer.Z();
          BitDotArr[0].ALL_DOT_BRIGHTNESS(Controller.getPRReading());
        }

        void manageGravityMode() {
          if (pow(y, 2) > tippingPoint || pow(z, 2) > tippingPoint) {
            GravityMode = true;
            GravityModeTimer = 1;
          } else {
            GravityModeTimer++;
          }
          if (!isAClock() && GravityModeTimer > TimeLimit) {
            GravityMode = false;
            resetClock();
          }
        }

        void manageDisplayMode() {
          int8_t mode = Controller.getMode();
          DotsInUse = ClockDisplays[mode]->requestNumOfDots();
          ClockDisplays[mode]->updateTime(RTC.now());
        }

        void manageTimeAdjust() {
          int8_t tempH = Controller.getHourUpdate();
          int8_t tempM = Controller.getMinUpdate();
          if (tempH != 0 || tempM != 0) {
            DateTime now = RTC.now();
            DateTime update = DateTime(now.year(), now.month(), now.day(), now.hour() + tempH, now.minute() + tempM, now.second());
            RTC.adjust(update);
          }
        }

        void manageController() {
          switch (Controller.buttonCheck()) {
            case Action::TimeAdjust:
              Controller.stayInAdjust();
              manageTimeAdjust();
              break;

            case Action::ModeChange:
              int8_t mode = Controller.getMode();
              cleanSlate(PaletteArr[Controller.getPalIndex()]);
              ClockDisplays[mode]->buildClock();
              break;
          
            case Action::DoNothing:
              break;
          }
        }

        void cleanSlate(TProgmemRGBGradientPalettePtr palPoint) {
          BitDotArr[0].CLEAN_ALL_LOCATIONS();
          for(int i = 0; i < BD_NUM; ++i) {
            BitDotArr[i].hardReset();
          }
          MasterPal = palPoint;
          ClockDisplay::PalettePointer = MasterPal;
        }

        bool isAClock() {
          for (int i = 0; i < DotsInUse; ++i) {
            if (!BitDotArr[i].isInClockFormation()) {
              return false;
            }
          }
          return true;
        }

        void resetClock() {
          BitDotArr[0].CLEAN_ALL_LOCATIONS();
          for (int i = 0; i < DotsInUse; ++i) {
            BitDotArr[i].setToClockPosition();
          }
        }

        void refreshDots() {
          CRGB temp = ColorFromPalette(BackGroundPal, Controller.getBGIndex(), BG_BRIGHT);
          BitDotArr[0].DISPLAY_BACKGROUND(temp);
          BitDotArr[0].SHOW_ALL_DOTS();
          BitDotArr[0].CLEAR_ALL_DOTS();
        }

};


CompleteClock TheClock(BUTT_1, BUTT_2, BUTT_3, PR);

void setup() {
    Wire.begin();
    TheClock.begin();
    Serial.begin(115200);
}

void loop() {
  TheClock.runClock();
}

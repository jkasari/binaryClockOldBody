#include <FastLED.h>
#include "RTClib.h"
#include <Wire.h>
#include <SparkFunADXL313.h>

#define LED_PIN 6 // Pin for the led matrix data output
#define LED_NUM 64 // Number of leds in the matrix
#define BD_NUM 24 // Number of total bit dots needed for all displays.
#define PR A0 // Photoresitor data in
#define BUTT_1 5 // Button 1
#define BUTT_2 4 // Button 2
#define BUTT_3 3 // Button 3
#define MODE_LIM 3 // Limit of display modes.
#define BACKG_NUM 9 // Number of backgrounds  
#define ACCEL_PORT 0x69 // Wire address of the accelerometer
#define TIP_POINT 40000 // At what point the dots ball out of place.
#define FADE_RATE 2 // Rate at which dots fade colors (has to be a multiple of 2)
#define RESET 500 // Time before the clock resest the dots
#define DOT_MOVE 900 // The resistence for dot movment
#define B_HIGH 200 // Brightness high limit
#define B_LOW 2 // Brightness low limit
#define BLANK CRGB(0,0,0) // A blank CRGB object.
#define SIX_BIT 6 // Literally the number six.
#define FOUR_BIT 4 // Literally the number 4
#define BYTE 8 // Literally the number 8


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
    0, 100, 0, 0,
    33, 75, 25, 0,
    66, 100, 0, 0,
    99, 50, 15, 0,
    132, 75, 15, 0,
    185, 80, 0, 0,
    208, 100, 20, 0,
    255, 70, 10, 0

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

class Button{

    public:
        Button(uint8_t port) {
            Port = port;
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

enum class Action{ ModeChange, TimeAdjust, DoNothing, BGChange};

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
        
        Action buttonCheck() { // Returns true if it's time to go into time adjust mode
            if (!MainButt.isPressed() && MainButt.getCount() > 1) {
              uint32_t count = MainButt.getCount();
              MainButt.clearCount();
              if (HalfSecond > count) {Mode++; modeLimitCheck(); return Action::ModeChange;}
              if (count > HalfSecond && MultiSecond > count) {Mode--; modeLimitCheck(); return Action::ModeChange;}
              if (count > MultiSecond) {adjustMode = !adjustMode;}
            }
            if (!HourButt.isPressed() && HourButt.getCount() > 1) {
              uint32_t count = HourButt.getCount();
              HourButt.clearCount();
              if (HalfSecond > count) {BGIndex++; BGLimitCheck(); return Action::BGChange;}
              if (count > HalfSecond && MultiSecond > count) {BGIndex--; BGLimitCheck(); return Action::BGChange;}
            }
            if (adjustMode) {
              return Action::TimeAdjust;
            } else {
              return Action::DoNothing;
            }
        }

        //int8_t getHourUpdate() {
        //    uint32_t count = HourButt.check();
        //    if (count > 1) {
        //        if (HalfSecond > count) {return 1;}
        //        if (count > HalfSecond) {return -1;}
        //    }
        //    return 0;
        //}

        //int8_t getMinuteUpdate() {
        //    uint32_t count = MinButt.check();
        //    if (count > 1) {
        //        if (HalfSecond > count) {return 1;}
        //        if (count > HalfSecond) {return -1;}
        //    }
        //    return 0;
        //}

        int8_t getPRReading() {
          return PR_Reader.getValue();
        }
        

    private:
        Button MainButt;
        Button HourButt;
        Button MinButt;
        PhotoResistorSmoother PR_Reader; 
        const uint16_t HalfSecond = 500;
        const uint16_t MultiSecond = 5000;
        int8_t Mode = 0;
        int8_t BGIndex = 0;
        bool adjustMode = false;
        void modeLimitCheck() {
            if (Mode >= MODE_LIM) {
                Mode = 0;
            } 
            if (0 > Mode) {
                Mode = MODE_LIM - 1;
            }
        }

        void BGLimitCheck() {
            if (BGIndex >= BACKG_NUM) {
                BGIndex = 0;
            } 
            if (0 > BGIndex) {
                BGIndex = BACKG_NUM - 1;
            }
        }

};

/**
 * The idea with this class was that it would be called in a loop and give you a different color in
 * out of a palette each time it was called.
 * It has a palette point and an indexing data member.
 * You can also create a background with a solid color and it will just return the same color each time
 * it is called. 
 * 
 */
class BackGround{
  public:
      // Doesn't work
      void createBackGround(CRGBPalette16 *palPoint, uint8_t rate) {
        ColorPalette = palPoint;
        Rate = rate;
      }

      void createBackGround(CRGB color) {
        SolidColor = color;
      }

      CRGB getColor() {
        if (Rate > 0) {
          EVERY_N_MILLISECONDS(Rate) {
            Increment++;
            if (Increment % Rate == 0) {
              return ColorFromPalette(*ColorPalette, Increment);
            }
          }
        }
        return SolidColor;
      }

  private:
      uint8_t Increment; 
      uint8_t Rate = 0;
      CRGBPalette16 *ColorPalette;
      CRGB SolidColor = BLANK;
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

        void DISPLAY_BACKGROUND(BackGround &BackG) {
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

          void displayBackground(int8_t location, BackGround &BGColor) {
            FLEDS[location] = BGColor.getColor();
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


class TestClock:ClockDisplay{
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
 * This displays time horizontally in 16 bits.
 * two 6 bit arrays handle hours and minutes with the remaining 4 showng hours.
 * It displays white for 0 and yellow for 1. 
 * 
 */
class SixteenBitWhiteClock:ClockDisplay{
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
 * Same as the other 16 bit display only this time the hours minutes and seconds each get
 * their own unique color. 
 * 
 */
class SixteenBitMultiColClock:ClockDisplay{
  public:
    void buildClock() {
      setSecondsIndex(0);
      setMinutesIndex(6);
      setHoursIndex(12);
      setDotsNeeded(16);
      buildBitDigitHorizontal(1, 6, getSecondsIndex(), uint8_t(SIX_BIT));
      buildBitDigitHorizontal(3, 6, getMinutesIndex(), uint8_t(SIX_BIT));
      buildBitDigitHorizontal(6, 5, getHoursIndex(), uint8_t(FOUR_BIT));
      for (int i = 0; i < 16; ++i) {
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

class ThreeByteColorClock:ClockDisplay{
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
          Accelerometer.measureModeOn();
          BitDotArr[0].begin();
          RTC.begin();
          createBackGrounds();
          MasterPal = MAP_YLWWHITE;
          ClockDisplay::BitDotPointer = BitDotArr;
          ClockDisplay::PalettePointer = MasterPal;
          Clock16Bit.buildClock();
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
        SixteenBitWhiteClock Clock16Bit;
        SixteenBitMultiColClock ColorClock16Bit;
        ThreeByteColorClock ThreeByteClock;
        TestClock TestBoi;
        ControlBoard Controller;
        ADXL313 Accelerometer;
        CRGBPalette16 MasterPal;
        bool GravityMode = false;
        int16_t x;
        int16_t y;
        uint8_t DotsInUse = BD_NUM;
        uint16_t tippingPoint = TIP_POINT;
        uint32_t GravityModeTimer = 0;
        uint32_t TimeLimit = RESET;
        BackGround BackGArr[BACKG_NUM];

        void readInputs() {
          if(Accelerometer.dataReady()) {
            Accelerometer.readAccel(); // read all 3 axis, they are stored in class variables: myAdxl.x, myAdxl.y and myAdxl.z
          }
          x = Accelerometer.x;
          y = Accelerometer.y;
          BitDotArr[0].ALL_DOT_BRIGHTNESS(Controller.getPRReading());
        }

        void manageGravityMode() {
          if (pow(y, 2) > tippingPoint) {
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
          switch(Controller.getMode()) {
            case 0:
              DotsInUse = Clock16Bit.requestNumOfDots();
              Clock16Bit.updateTime(RTC.now());
              break;
            
            case 1:
              DotsInUse = ColorClock16Bit.requestNumOfDots();
              ColorClock16Bit.updateTime(RTC.now());
              break;

            case 2:
              DotsInUse = ThreeByteClock.requestNumOfDots();
              ThreeByteClock.updateTime(RTC.now());
              break;
          }
        }

        void manageController() {
          switch (Controller.buttonCheck()) {
            case Action::ModeChange:
              switch (Controller.getMode()) {
                case 0:
                  cleanSlate(MAP_YLWWHITE);
                  Clock16Bit.buildClock();
                  break;
                case 1:
                  cleanSlate(Rainbow_gp);
                  ColorClock16Bit.buildClock();
                  break;
                case 2:
                  cleanSlate(Rainbow_gp);
                  ThreeByteClock.buildClock();
                  break;
              }
              break;
            
            case Action::TimeAdjust:
              break;
          
            case Action::DoNothing:
              break;
          }
        }

        void beginDisplay() {
          
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
          BitDotArr[0].DISPLAY_BACKGROUND(BackGArr[Controller.getBGIndex()]);
          BitDotArr[0].SHOW_ALL_DOTS();
          BitDotArr[0].CLEAR_ALL_DOTS();
        }

        void createBackGrounds() {
          BackGArr[0].createBackGround(CRGB(0, 0, 0));
          BackGArr[1].createBackGround(CHSV(0, 255, 70));
          BackGArr[2].createBackGround(CHSV(32, 255, 70));
          BackGArr[3].createBackGround(CHSV(64, 255, 70));
          BackGArr[4].createBackGround(CHSV(96, 255, 70));
          BackGArr[5].createBackGround(CHSV(128, 255, 70));
          BackGArr[6].createBackGround(CHSV(150, 255, 70));
          BackGArr[7].createBackGround(CHSV(192, 255, 70));
          BackGArr[8].createBackGround(CHSV(224, 255, 70));
        }
};


CompleteClock TheClock(BUTT_1, BUTT_2, BUTT_3, PR);

void setup() {
    pinMode(BUTT_1, INPUT_PULLUP);
    pinMode(BUTT_2, INPUT_PULLUP);
    pinMode(BUTT_3, INPUT_PULLUP);
    Wire.begin();
    TheClock.begin();
    Serial.begin(115200);
}

void loop() {
  TheClock.runClock();
}

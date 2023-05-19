#include <Arduino.h>
#include <Wire.h>
#include <AM2320.h>
///
/// http://www.esp8266learning.com/am2320-temperature-humidity-sensor-esp8266-example.php
///
#include "buzzer.h"
#include "consts.h"
#if __has_include("secret_real.h")
  #include "secret_real.h"
#else
  #include "secret_dummy.h"
#endif

class SensorAM2320Manager {
  Buzzer *buzzer;
  AM2320 *sensor;

public:
    int lastSuccessFreq;
    float lastTemp;
    float lastHum;

    SensorAM2320Manager() {
      lastSuccessFreq = 0;
      Wire.begin(PIN_AM2320_I2C_SDA, PIN_AM2320_I2C_SCL);
      sensor = new AM2320(&Wire);

      buzzer = new Buzzer(PIN_BUZZER);
    }

    ~SensorAM2320Manager() {
      delete sensor;
      delete buzzer;
    }

    //------------------------------------------
    // Try to read data from the sensor AM2320 with selection clock frequency
    // Return result code:
    //    true if new data successfull readed
    //    false otherwise
    bool tryToReadData () {
      Serial.println("\n\nTry to receive new data from AM2320 sensor.");
      lastTemp = 0.0;
      lastHum = 0.0;

      int startFreq = lastSuccessFreq > 0? lastSuccessFreq: I2C_CLOCK_FREQ_MAX;

      // Go down
      bool resReading = scanFrequenciesForReadData(startFreq, I2C_CLOCK_FREQ_MIN, -I2C_CLOCK_FREQ_CHANGE_STEP);
      if(!resReading && lastSuccessFreq > 0) {
        // Go Up
        resReading = scanFrequenciesForReadData(lastSuccessFreq, I2C_CLOCK_FREQ_MAX, I2C_CLOCK_FREQ_CHANGE_STEP);

        if(!resReading) {
          lastSuccessFreq = 0;
        }
      }
      return resReading;
    }


    bool scanFrequenciesForReadData(int frStart, int frEnd, int frStep) {
      for(int freq = frStart; (frStep>0 ? freq<=frEnd : freq>=frEnd); freq += frStep) {
        Wire.setClock(freq);

        uint8_t readState = sensor->Read();
        Serial.printf("  Frequency %u Hz. Result state= %d: ", freq, readState);

        if(handleReadDataState(readState)) {
          lastSuccessFreq = freq;
          return true;
        }
        // Squeaking if failure
        buzzer->beep(freq/4, AM2320_DELAY_BETWEEN_READING_ATTEMPTS/4);
        // Pause between attempts
        delay(3*AM2320_DELAY_BETWEEN_READING_ATTEMPTS/4);
      }
      return false;
    }
    
    bool handleReadDataState(uint8_t readingState) {
        switch(readingState) {
          case 0:
            lastTemp = sensor->cTemp;
            lastHum = sensor->Humidity;
            Serial.printf(" Humidity = %.1f%%,  Temperature = %.1f*C\n", lastHum, lastTemp);
            return true;
          break;
          case 4: // line busy
            Serial.println(" line busy");
          break;
          case 3: // received NACK (not acknowledging the slave address) on transmit of DATA
            Serial.println(" received NACK on transmit of DATA"); // NACK -  not acknowledging the slave address
          break;
          case 2: // CRC failed... Try Again
            Serial.println(" CRC failed / received NACK on transmit of ADDRESS");
          break;
          case 1: // Sensor offline
            Serial.println(" offline");
            // return AM3220_READ_DATA_RESULT_OFFLINE;
          break;
        }
        return false;
    }
};
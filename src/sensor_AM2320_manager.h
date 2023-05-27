#include <Arduino.h>
#include <Wire.h>
///
/// http://www.esp8266learning.com/am2320-temperature-humidity-sensor-esp8266-example.php
///
#include <AM2320.h>
#include "sensors_manager_interface.h"
#include "buzzer.h"
#include "consts.h"

#define SENSOR_NAME_TEMPERAT "AM2320_T"
#define SENSOR_NAME_HUMIDITY "AM2320_H"

#define ISENS_TMP 0
#define ISENS_HUM 1

class SensorAM2320Manager_: public ISensorsManager {
    private:
        Buzzer *buzzer;
        AM2320 *sensor;
        int lastSuccessFreq;

    public: 
        SensorAM2320Manager_() {
            buzzer = new Buzzer(PIN_BUZZER);

            lastSuccessFreq = 0;
            Wire.begin(PIN_AM2320_I2C_SDA, PIN_AM2320_I2C_SCL);
            sensor = new AM2320(&Wire);

            numSensors = 2;
            sensNames = new String[numSensors];
            sensNames[ISENS_TMP] = SENSOR_NAME_TEMPERAT;
            sensNames[ISENS_HUM] = SENSOR_NAME_HUMIDITY;

            lastData = new float[numSensors];
            lastData[ISENS_TMP] = lastData[ISENS_HUM] = 0.0;
        }

        ~SensorAM2320Manager_() {
            delete buzzer;
            delete sensor;
            delete[] sensNames;
            delete[] lastData;
        }

        bool checkSensorsListChanged() {
            if(sensListChanged) {
                sensListChanged = false;
                return true;
            }
            return false;
        }

        //------------------------------------------
        // Try to read data from the sensor AM2320 with selection clock frequency
        // Return result code:
        //    true if new data successfull readed
        //    false otherwise
        bool tryToReadData() {
            Serial.println("\n\nTry to read data from AM2320 sensor.");
            int startFreq = lastSuccessFreq > 0? lastSuccessFreq: I2C_CLOCK_FREQ_MAX;
            // Let's try to read the data. In case of failure, gradually REDUCE the I2C frequency.
            hasLastData = scanFrequenciesForReadData(startFreq, I2C_CLOCK_FREQ_MIN, -I2C_CLOCK_FREQ_CHANGE_STEP);
            if(!hasLastData && lastSuccessFreq > 0) {
                // Now, gradually INCREASE the I2C frequency.
                hasLastData = scanFrequenciesForReadData(lastSuccessFreq, I2C_CLOCK_FREQ_MAX, I2C_CLOCK_FREQ_CHANGE_STEP);

                if(!hasLastData) {
                    lastSuccessFreq = 0;
                }
            }
            return hasLastData;
        }

    private:

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
                case 0: {
                    float lastTemp = sensor->cTemp;
                    float lastHum = sensor->Humidity;
                    lastData[ISENS_TMP] = lastTemp;
                    lastData[ISENS_HUM] = lastHum;
                    Serial.printf(" Humidity = %.1f%%,  Temperature = %.1f*C\n", lastHum, lastTemp);
                    return true;
                }
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
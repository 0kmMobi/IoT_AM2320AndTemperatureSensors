#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "sensors_manager_interface.h"
#include "buzzer.h"
#include "consts.h"

class SensorsDS18B20Manager_: public ISensorsManager {
    private:
        Buzzer *buzzer;
        OneWire *oneWire;
        DallasTemperature *DS18B20;
        DeviceAddress *sensAddrs = NULL;  //An array temperature sensors ids

    public:
        SensorsDS18B20Manager_() {
            buzzer = new Buzzer(PIN_BUZZER);

            oneWire = new OneWire(PIN_ONE_WIRE_BUS);
            DS18B20 = new DallasTemperature(oneWire);
            Serial.printf("\nInitialization OneWire Sensors.\n");
            DS18B20->begin();
            //Serial.printf("Parasite power is: %s.\n", DS18B20->isParasitePowerMode()?"ON":"OFF"); 
            DS18B20->setWaitForConversion(true);

            //DS18B20->requestTemperatures();
            //Serial.printf("Number of found sensors: %d.\n", numSensors);
            initSensors(DS18B20->getDS18Count(), true);
        }

        ~SensorsDS18B20Manager_() {
            delete buzzer;
            delete oneWire;
            delete DS18B20;
            delete[] sensAddrs;
            delete[] sensNames;
            delete[] lastData;
        }

        bool checkSensorsListChanged() {
            DS18B20->requestTemperatures();

            //uint8_t curNumSensors = DS18B20->getDS18Count();
            uint8_t curNumSensors = getRealConnectedSensors();

            if(numSensors != curNumSensors) {
                Serial.printf("Number of found sensors was changed: %d -> %d.\n", numSensors, curNumSensors);
                sensListChanged = true;
                initSensors(curNumSensors, true);
            } else {
                bool needParticalyReinit = false;
                DeviceAddress addr;
                // Loop for all devices, print out address
                for(int i = 0; i < numSensors; i++) {
                    //sensListChanged |= !DS18B20->isConnected(sensAddrs[i]);
                    if(DS18B20->getAddress(addr, i) && DS18B20->isConnected(addr) && areAddressesEqual(addr, sensAddrs[i])) {

                    } else {
                        needParticalyReinit |= true;
                    }
                }

                if(needParticalyReinit) {
                    Serial.println("One or more sensors online was changed");
                    initSensors(numSensors, false);
                }
            }

            if(sensListChanged) {
                sensListChanged = false;
                return true;
            }
            return false;
        }


        bool tryToReadData() {
            hasLastData = false;
            if(numSensors == 0) {
                buzzer->beep(250, 100);
                delay(100);
                buzzer->beep(200, 100);
                delay(100);
                buzzer->beep(150, 100);
                return false;
            }
            Serial.println("Reading data from DS18B20 sensors");

            for(int i = 0; i < numSensors; i++) {
                lastData[i] = DS18B20->getTempCByIndex(i);
                Serial.printf("  Sensor #%s temperature = %.1f*C\n", sensNames[i].c_str(), lastData[i]);
            }
            hasLastData = true;
            return true;
        }


    private:
        uint8_t getRealConnectedSensors() {
            DeviceAddress deviceAddress;
	        oneWire->reset_search();
	        uint8_t ds18Count = 0; // Reset number of DS18xxx Family devices

	        while (oneWire->search(deviceAddress)) {
                if (DS18B20->validAddress(deviceAddress) && DS18B20->validFamily(deviceAddress)) {
                    ds18Count++;
                }
            }
            return ds18Count;
        }

        void initSensors(int curNumSensors, bool fullInit) {
            if(fullInit) {
                DS18B20->begin();
                //Serial.printf("Parasite power is: %s.\n", DS18B20->isParasitePowerMode()?"ON":"OFF"); 
                DS18B20->setWaitForConversion(true);

                if(sensAddrs != NULL)   delete[] sensAddrs;
                if(sensNames != NULL)   delete[] sensNames;
                if(lastData != NULL)    delete[] lastData;
                numSensors = curNumSensors;
                sensAddrs = new DeviceAddress[numSensors];
                sensNames = new String[numSensors];
                lastData = new float[numSensors];
            }

            for(int i = 0; i < numSensors; i++) {
                if( DS18B20->getAddress(sensAddrs[i], i) && DS18B20->isConnected(sensAddrs[i]) ) {
                    sensNames[i] = sensAddrToStr(sensAddrs[i]);
                    Serial.printf("Device at #%d with address: %s.\n", i, sensNames[i].c_str());
                } else {
                    sensNames[i].clear(); // = NULL;
                    Serial.printf("!!! Ghost device at %d, but could not detect address. Check power and cabling.\n", i);
                }
                // // Get resolution of DS18b20
                // Serial.printf("  Resolution: %d bits.\n", DS18B20->getResolution( sensAddrs[i] ));
            }
            buzzer->beep(200, 500);
        }


        // Convert device id to String
        String sensAddrToStr(DeviceAddress oneSensorAddr) {
            String str = "";
            for(uint8_t i = 0; i < 8; i++) {
                if( oneSensorAddr[i] < 16 ) 
                    str += String(0, HEX);
                str += String(oneSensorAddr[i], HEX);
            }
            return str;
        }

        bool areAddressesEqual(DeviceAddress addr1, DeviceAddress addr2) {
            for(uint8_t i = 0; i < 8; i++) {
                if(addr1[i] != addr2[i])
                    return false;
            }
            return true;
        }
};
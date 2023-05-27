
#ifndef SENSORS_MANAGER_INTERFACE
#define SENSORS_MANAGER_INTERFACE

#include <Arduino.h>

class ISensorsManager {
    public:
        bool hasLastData = false;

        virtual bool checkSensorsListChanged() = 0;
        virtual bool tryToReadData() = 0; // Tries to read data from sensors

        int getNumSensors() {
            return numSensors;
        }

        String* getActualSensorsNames() {
            return sensNames;
        }

        float* getActualSensorsData() {
            return lastData;
        }

    protected:
        int numSensors = 0;
        String* sensNames = NULL;
        bool sensListChanged = true;
        float* lastData = NULL;
};

#endif // SENSORS_MANAGER_INTERFACE
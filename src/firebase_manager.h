
#ifndef FIREBASE_MANAGER_H
#define FIREBASE_MANAGER_H

#if __has_include("secret_real.h")
  #include "secret_real.h"
#else
  #include "secret_dummy.h"
#endif

#include <Firebase_ESP_Client.h>
//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

#include "sensors_manager_interface.h"
#include "consts.h"


  static bool needToSendPong = false;
  FirebaseData stream;

  void streamCallback(FirebaseStream data) {
    String sDataPath = data.dataPath();
    String sDataType = data.dataType();

    Serial.printf("*** sream event path: %s; data type: %s; intData: %d\n",
                  // data.streamPath().c_str(),
                  sDataPath.c_str(),
                  sDataType.c_str(),
                  // data.eventType().c_str(),
                  data.intData());
    printResult(data); // see addons/RTDBHelper.h

    if(sDataType.equals("json")) {
      FirebaseJson *json = data.jsonObjectPtr();
      size_t jsonLen = json->iteratorBegin();
      FirebaseJson::IteratorValue jsonIterValue;

      for (size_t i = 0; i < jsonLen; i++) {
        jsonIterValue = json->valueAt(i);
        String sKey = jsonIterValue.key;
        String sValue = jsonIterValue.value;
        if(sKey.equals(PARAM_NAME_DURATION)) {
          int seconds;
          sscanf(sValue.c_str(), "%d", &seconds);
          setDataDuration( seconds );
        }
      }
      json->iteratorEnd();
    } else {
      String sParamName = sDataPath.substring(1);
      if(sDataType.equals("int")) {
        if(sParamName.equals(PARAM_NAME_DURATION)) {
          setDataDuration( data.intData() );
        }
      } else if(sDataType.equals("double")) {
        if(sParamName.equals(PARAM_NAME_PING_TS)) {
          needToSendPong = true;
        }
      }
    }
    Serial.println();
    // This is the size of stream payload received (current and max value)
    // Max payload size is the payload size under the stream path since the stream connected
    // and read once and will not update until stream reconnection takes place.
    // This max value will be zero as no payload received in case of ESP8266 which
    // BearSSL reserved Rx buffer size is less than the actual stream payload.
    //      Serial.printf("*** PING Received stream payload size: %d (Max. %d)\n\n", data.payloadLength(), data.maxPayloadLength());
  }


  void streamTimeoutCallback(bool timeout) {
    if (timeout)
      Serial.println("*** stream timed out, resuming...\n");

    if (!stream.httpConnected())
      Serial.printf("*** error code: %d, reason: %s\n\n", stream.httpCode(), stream.errorReason().c_str());
  }


class FirebaseManager {
  //Define Firebase Data object
  String deviceMAC;
  FirebaseData fbdo;
  FirebaseAuth auth;
  FirebaseConfig config;

  

  void initPingStream() {
      Firebase.setDoubleDigits(5);
      Serial.println("  Initialization 'Ping' stream... ");

#if defined(ESP8266)
      stream.setBSSLBufferSize(2048 /* Rx in bytes, 512 - 16384 */, 512 /* Tx in bytes, 512 - 16384 */);
#endif
      //String path = DB_DEVICES_IDS_TYPES + deviceMAC + "/ping_last_ts";
      String path = DB_DEVICES_IDS_TYPES + deviceMAC + "/params";

      if (!Firebase.RTDB.beginStream(&stream, path))
        Serial.printf("    sream begin error, %s\n\n", stream.errorReason().c_str());

      Firebase.RTDB.setStreamCallback(&stream, streamCallback, streamTimeoutCallback);

      /** Timeout options, below is default config.
      //WiFi reconnect timeout (interval) in ms (10 sec - 5 min) when WiFi disconnected.
      config.timeout.wifiReconnect = 10 * 1000;
      //Socket begin connection timeout (ESP32) or data transfer timeout (ESP8266) in ms (1 sec - 1 min).
      config.timeout.socketConnection = 30 * 1000;
      //ESP32 SSL handshake in ms (1 sec - 2 min). This option doesn't allow in ESP8266 core library.
      config.timeout.sslHandshake = 2 * 60 * 1000;
      //Server response read timeout in ms (1 sec - 1 min).
      config.timeout.serverResponse = 10 * 1000;
      //RTDB Stream keep-alive timeout in ms (20 sec - 2 min) when no server's keep-alive event data received.
      config.timeout.rtdbKeepAlive = 45 * 1000;
      //RTDB Stream reconnect timeout (interval) in ms (1 sec - 1 min) when RTDB Stream closed and want to resume.
      config.timeout.rtdbStreamReconnect = 1 * 1000;
      //RTDB Stream error notification timeout (interval) in ms (3 sec - 30 sec). It determines how often the readStream
      //will return false (error) when it called repeatedly in loop.
      config.timeout.rtdbStreamError = 3 * 1000;
      */
  }

  public:
    FirebaseManager(String mac) {
      Serial.printf("\nInitialization Firebase Client v%s.\n", FIREBASE_CLIENT_VERSION);
      deviceMAC = mac;

      /* Assign the api key (required) */
      config.api_key = API_KEY;
      /* Assign the RTDB URL (required) */
      config.database_url = DATABASE_URL;

      // For the following credentials, see examples/Authentications/SignInAsUser/EmailPassword/EmailPassword.ino
      /* Assign the user sign in credentials */
      auth.user.email = USER_EMAIL;
      auth.user.password = USER_PASSWORD;  

      fbdo.setResponseSize(4096);

      /* Assign the callback function for the long running token generation task */
      config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

      /* Initialize the library with the Firebase authen and config */
      Firebase.begin(&config, &auth);
      Firebase.reconnectWiFi(true);

      initPingStream();
      Serial.println("Firebase Client initialized");
    }


    void sendDeviceInfo() {
      //String path = DB_DEVICES_IDS_TYPES + deviceMAC;
      String path = DB_DEVICES_IDS_TYPES + deviceMAC + "/type";
      Serial.printf("sendIOTDeviceInfo: Path = %s\n", path.c_str());
      
      bool result = Firebase.RTDB.setInt(&fbdo, path, IOT_TYPE);
      Serial.printf("sendIOTDeviceInfo: result = %s\n", result ? "ok" : fbdo.errorReason().c_str());
    }

    void sendSensorsList(int allSensors, ISensorsManager** sensManagers, int numSensManagers) {
        String path = DB_DEVICES_IDS_TYPES + deviceMAC + DB_SENSORS_NAMES;

        String* allNames = new String[allSensors];
        int index = 0;
        for(int iMan = 0; iMan < numSensManagers; iMan ++) {
          ISensorsManager* sensMan = sensManagers[iMan];
          String* names = sensMan->getActualSensorsNames();
          int numSens = sensMan->getNumSensors();
          for(int iSens = 0; iSens < numSens; iSens++) {
            allNames[index++] = names[iSens];
          }
        }
        // Create new JSON Object and add to it all actual and old sensors
        FirebaseJson jsonMapSensors;

        // Read from DB sensors list which were stored them there
        bool getJSONRes = Firebase.RTDB.getJSON(&fbdo, path);
        int resCode = fbdo.httpCode();

        if (getJSONRes && resCode == FIREBASE_ERROR_HTTP_CODE_OK) {
          FirebaseJson *jsonMapDB = fbdo.to<FirebaseJson *>();
          size_t jsonArrDBLen = jsonMapDB->iteratorBegin();

          for (size_t iDB = 0; iDB < jsonArrDBLen; iDB++) {
            String sSensorAddrDB = jsonMapDB->valueAt(iDB).key;
            jsonMapSensors.set(sSensorAddrDB, false);
          }
          jsonMapDB->iteratorEnd();
        }

        for(uint8_t iReal = 0; iReal < allSensors; iReal++) {
          String sSensorNameReal = allNames[iReal];
          jsonMapSensors.set(sSensorNameReal, true);
        }

        delete[] allNames;
        jsonMapSensors.toString(Serial, true);
        Serial.println();

        // The total sensors list send to DB
        bool jsonSetResult = Firebase.RTDB.set(&fbdo, path, &jsonMapSensors);
        Serial.printf("Set sensors names list to RTDB... %s\n", jsonSetResult ? "ok" : fbdo.errorReason().c_str());
    }

    void sendSensorsDataToDB(int allSensors, ISensorsManager** sensManagers, int numSensManagers) {
      Serial.println(" Push All sensors new data to DB.");
      String path = DB_DEVICES_DATA + deviceMAC + DB_SENSORS_VALUES;

      // Create new JSON Object and add to it all actual and old sensors
      FirebaseJson jsonSensors;

      for(int iMan = 0; iMan < numSensManagers; iMan ++) {
        ISensorsManager* sensMan = sensManagers[iMan];
        if(sensMan->hasLastData) {
          int numSens = sensMan->getNumSensors();
          String* names = sensMan->getActualSensorsNames();
          float* values = sensMan->getActualSensorsData();
          for(int iSens = 0; iSens < numSens; iSens++) {
            jsonSensors.set(names[iSens], values[iSens]);
          }
        }
      }

      FirebaseJson jsonTime;
      jsonTime.set(".sv", "timestamp");
      jsonSensors.set("time", jsonTime);

      bool result = Firebase.RTDB.pushAsync(&fbdo, path, &jsonSensors);
      Serial.printf("  Push data to RTDB ... %s\n\n", result ? "ok" : fbdo.errorReason().c_str());
    }


    void checkIfNeedSendPong() {
      if(!needToSendPong)
        return;
      needToSendPong = false;

      String path = DB_DEVICES_IDS_TYPES + deviceMAC + "/params/" + PARAM_NAME_PONG_TS;
      FirebaseJson json;
      FirebaseJson jsonTime;

      jsonTime.set(".sv", "timestamp");
      json.set("time", jsonTime);

      bool result = Firebase.RTDB.setJSON(&fbdo, path, &jsonTime);
      Serial.printf("*** Send PONG to RTDB ... %s\n\n", result ? "ok" : fbdo.errorReason().c_str());
    }
};

#endif /* FIREBASE_MANAGER_H */
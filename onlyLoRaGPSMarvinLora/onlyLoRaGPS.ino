#include <sps30.h>
//#include <Multichannel_Gas_GMXXX.h>
//#include <SoftwareWire.h>
#include <SoftwareSerial.h>
//#include <TinyGPS++.h>
#include <thingsml.h>
//#include <Adafruit_GPS.h>

#define DEBUG
#define SERIAL_BAUD_RATE 9600
#define LED_PIN 13

#define APPLICATION_PORT 1 // Port to assign the type of lora data (any port can be used between 1 and 223)

#define MICROCHIP_BAUD_RATE 57600
#define MICROCHIP_POWER_PORT 6 // Note that an earlier version of the Marvin doesn't support seperate power to RN2483
#define MICROCHIP_RESET_PORT 5

/**
 * Configuration
 */
const String DevEUI = "70B3D57ED0052D07";
const String AppEUI = "0000000000000000";
const String AppKey = "621B6591984AEFDA7A628C60F06294D7";


//SoftwareWire myWire(12, 4);
//GAS_GMXXX<SoftwareWire> gas;
//TinyGPSPlus gps;
//SoftwareSerial GPSSerial(14, 16);
//Adafruit_GPS GPS(&GPSSerial);
//SoftwareSerial mySerial(14, 16);
bool ForceReconfigureMicrochip = true; 

void print(String message)
{
    Serial.println(message);
}

void initMicrochip()
{
    

    Serial1.begin(MICROCHIP_BAUD_RATE);

    pinMode(MICROCHIP_POWER_PORT, OUTPUT);
    digitalWrite(MICROCHIP_POWER_PORT, HIGH);

    // Disable reset pin
    pinMode(MICROCHIP_RESET_PORT, OUTPUT);
    digitalWrite(MICROCHIP_RESET_PORT, HIGH);

    // reconfigure LoRa module from memory: restore band, deveui, appeui, appkey, nwkskey, appskey, devaddr, ch list
    sendToMicrochip("sys reset");
    readLineBlockingFromMicrochip();

    // Check if the saved devaddr is different from the desired devaddr. If so, reconfigure.
    

        sendToMicrochip("mac reset 868");
        readLineBlockingFromMicrochip();

        sendToMicrochip("mac set deveui " + DevEUI);
        readLineBlockingFromMicrochip();

        sendToMicrochip("mac set appeui " + AppEUI);
        readLineBlockingFromMicrochip();

        sendToMicrochip("mac set appkey " + AppKey);
        readLineBlockingFromMicrochip();

        // default SF12 on RX2
       sendToMicrochip("mac set rx2 3 868100000");
        readLineBlockingFromMicrochip();

        sendToMicrochip("mac save");
        readLineBlockingFromMicrochip();
    

    // full power (14dBm)
    sendToMicrochip("mac set pwridx 1");
    readLineBlockingFromMicrochip();

    // default SF7
    sendToMicrochip("mac set dr 5");
    // For SF12
//     sendToMicrochip("mac set dr 1");
    readLineBlockingFromMicrochip();

    // ADR on
    sendToMicrochip("mac set adr on");
    readLineBlockingFromMicrochip();
    #ifdef DEBUG
        print("Configuration correct!");
#endif
    bool joinSuccess = false;
    unsigned int joinTries = 0;
    while (!joinSuccess)
    {
        if ((joinTries++) > 0)
        {
#ifdef DEBUG
            print("Join failed. Retry in 30 seconds.");
#endif
            delay(30000);
        }

        sendToMicrochip("mac join otaa");
        if (readLineBlockingFromMicrochip() == "ok")
        {
            joinSuccess = (readLineBlockingFromMicrochip() == "accepted");
        }
        else
        {
            joinSuccess = false;
        }
    }

    print("Microchip configured");

}

void sendToMicrochip(String cmd)
{
    
    Serial1.println(cmd);
    print(" > " + cmd);
}



void send_LoRa_data(int tx_port, String rawdata)
{
  sendToMicrochip("mac tx uncnf " + String(tx_port) + String(" ") + rawdata);
  delay(100);
}
//
bool sendPayload(bool confirmed, int applicationPort, byte *payload, byte payloadLength)
{
    String str = "";
    char c[2];
    for (int i = 0; i < payloadLength; i++)
    {
        sprintf(c, "%02x", payload[i]);
        str += c;
    }
    return sendPayload(confirmed, applicationPort, str);
}

/*
 * Send a payload
 *
 * @param confirmed - wether the frame should be confirmed, or unconfirmed
 * @param applicationPort - the application port on which to send the payload
 * @param payload - the payload to be send, in hexadecimal representation
 *
 * @return whether the payload has successfully been transmitted
 */
bool sendPayload(bool confirmed, int applicationPort, String payload)
{
    String type, str, response;

    type = (confirmed) ? "cnf" : "uncnf";
    str = "mac tx " + type + " " + String(applicationPort) + " " + payload;
    sendToMicrochip(str);
    
    return true;
}

/*
 * Default to echo the string received
 */
String readLineBlockingFromMicrochip()
{
    
    char c;
    String in;

    while (!Serial1.available())
        ;
    while (Serial1.available())
    {
        c = Serial1.read();
        if (c != '\n' && c != '\r')
        {
            in += c;
        }

        if (c == '\n')
        {
            break;
        }
        delay(1);
    }
#ifdef DEBUG
    
        print(" < " + in);
    
#endif
    return in;
}



SenMLPack device;
//SenMLDoubleRecord latitude(THINGSML_LATITUDE);
//SenMLDoubleRecord longitude(THINGSML_LONGITUDE);
SenMLDoubleRecord pm_2_5(THINGSML_CO_CONCENTRATION);
SenMLDoubleRecord pm_10(THINGSML_CO2_CONCENTRATION);
char buf[100] = {0};


void setup()
{
  
    Serial.begin(4800);
    // If you have changed the I2C address of gas sensor, you must to be specify the address of I2C.
    //The default addrss is 0x08;
    delay(2000);
    // gas.begin(Wire, 0x04); // use the hardware I2C
//    gas.begin(myWire, 0x04); // use the software I2C
    //gas.setAddress(0x64); change thee I2C address

    int16_t ret;
    uint8_t auto_clean_days = 4;
    sensirion_i2c_init();
    if (sps30_probe() != 0)
    {
        //Serial.print("SPS sensor probing failed\n");
        delay(500);
    }
    //Serial.print("SPS sensor probing successful\n");
    ret = sps30_set_fan_auto_cleaning_interval_days(auto_clean_days);
    if (ret)
    {
        Serial.print("error setting the auto-clean interval: ");
    }
    ret = sps30_start_measurement();
    if (ret < 0)
    {
        Serial.print("error starting measurement\n");
    }
    Serial.print("measurements started\n");
    /*****GPS begin******/
//    GPSSerial.begin(9600);
//     GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
    // // uncomment this line to turn on only the "minimum recommended" data
     //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
    //GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
    //                                            //antenna status
 //GPS.sendCommand(PGCMD_ANTENNA);
    //delay(1000);
   // GPSSerial.println(PMTK_Q_RELEASE);
    /*****GPS end******/

//    pinMode(LED_PIN, OUTPUT); // Initialize LED port
//    digitalWrite(LED_PIN, HIGH);

//#ifdef DEBUG
//    Serial.begin(SERIAL_BAUD_RATE);
//    while (!Serial)
//        ;
//        print("Serial port initialised");
//#endif

    
//    device.add(longitude);
//    device.add(latitude);
    device.add(pm_2_5);
    device.add(pm_10);
    

    initMicrochip();
//    digitalWrite(LED_PIN, LOW);
}

// uint32_t timer = millis();

void loop()
{


    delay(2000);

    struct sps30_measurement m;
    uint16_t data_ready;
    int16_t ret;
    do
    {
        ret = sps30_read_data_ready(&data_ready);
        ret =10;
        if (ret < 0)
        {
            Serial.print("error reading data-ready flag: ");
            Serial.println(ret);
        }
        else if (!data_ready)
            Serial.print("data not ready, no new measurement available\n");
        else
            break;
        delay(100); /* retry in 100ms */
    } while (1);
    ret = sps30_read_measurement(&m);
    if (ret < 0)
    {
        Serial.print("error reading measurement\n");
    }
    else
    {
//        Serial.print("PM  1.0: ");
//        Serial.println(m.mc_1p0);
        Serial.println("PM  2.5: ");
        Serial.println(m.mc_2p5);
        pm_2_5.set(m.mc_2p5);
//        Serial.print("PM  4.0: ");
//        Serial.println(m.mc_4p0);
        Serial.print("PM 10.0: ");
        Serial.println(m.mc_10p0);
        pm_10.set(m.mc_10p0);
//        Serial.print("NC  0.5: ");
//        Serial.println(m.nc_0p5);
//        Serial.print("NC  1.0: ");
//        Serial.println(m.nc_1p0);
//        Serial.print("NC  2.5: ");
//        Serial.println(m.nc_2p5);
//        Serial.print("NC  4.0: ");
//        Serial.println(m.nc_4p0);
//        Serial.print("NC 10.0: ");
//        Serial.println(m.nc_10p0);
//        Serial.print("Typical partical size: ");
//        Serial.println(m.typical_particle_size);
    }
    

//     char c = GPS.read();
//     if (GPS.newNMEAreceived())
//     {
//         if (!GPS.parse(GPS.lastNMEA())) // this also sets the newNMEAreceived() flag to false
//             return;                     // we can fail to parse a sentence in which case we should just wait for another
//     }
//     printGPSData();

//    while (GPSSerial.available() > 0)
//      if(gps.encode(GPSSerial.read()))
//            displayGPSInfo();
//                                     
//      if (millis() > 5000 && gps.charsProcessed() < 10)
//      {
//          Serial.println(F("No GPS detected: check wiring."));
//          while (true);
//      }
    
//    device.toJson(Serial);
    device.toCbor(buf, sizeof(buf), SENML_HEX);
    sendPayload(false, 1, buf);
    delay(5000);
}

//void displayGPSInfo()
//{
//    Serial.print(F("Location: "));
//    if (gps.location.isValid())
//    {
//        Serial.print(gps.location.lat(), 6);
//        Serial.print(F(","));
//        Serial.print(gps.location.lng(), 6);
//        latitude.set(gps.location.lat());
//        longitude.set(gps.location.lng());
//    }
//    else
//    {
//        Serial.print(F("INVALID"));
//        
//    }

//    Serial.print(F("  Date/Time: "));
//    if (gps.date.isValid())
//    {
//        Serial.print(gps.date.month());
//        Serial.print(F("/"));
//        Serial.print(gps.date.day());
//        Serial.print(F("/"));
//        Serial.print(gps.date.year());
//    }
//    else
//    {
//        Serial.print(F("INVALID"));
//    }

//    Serial.print(F(" "));
//    if (gps.time.isValid())
//    {
//        if (gps.time.hour() < 10)
//            Serial.print(F("0"));
//        Serial.print(gps.time.hour());
//        Serial.print(F(":"));
//        if (gps.time.minute() < 10)
//            Serial.print(F("0"));
//        Serial.print(gps.time.minute());
//        Serial.print(F(":"));
//        if (gps.time.second() < 10)
//            Serial.print(F("0"));
//        Serial.print(gps.time.second());
//        Serial.print(F("."));
//        if (gps.time.centisecond() < 10)
//            Serial.print(F("0"));
////        Serial.print(gps.time.centisecond());
//    }
//    else
//    {
////        Serial.print(F("INVALID"));
//    }

//    Serial.println();
//}

//void printGPSData()
//{
//
//     // reset the timer
////        Serial.print("\nTime: ");
////        if (GPS.hour < 10)
////        {
////            Serial.print('0');
////        }
////        Serial.print(GPS.hour, DEC);
////        Serial.print(':');
////        if (GPS.minute < 10)
////        {
////            Serial.print('0');
////        }
////        Serial.print(GPS.minute, DEC);
////        Serial.print(':');
////        if (GPS.seconds < 10)
////        {
////            Serial.print('0');
////        }
////        Serial.print(GPS.seconds, DEC);
////        Serial.print('.');
////        if (GPS.milliseconds < 10)
////        {
////            Serial.print("00");
////        }
////        else if (GPS.milliseconds > 9 && GPS.milliseconds < 100)
////        {
////            Serial.print("0");
////        }
////        Serial.println(GPS.milliseconds);
////        Serial.print("Date: ");
////        Serial.print(GPS.day, DEC);
////        Serial.print('/');
////        Serial.print(GPS.month, DEC);
////        Serial.print("/20");
////        Serial.println(GPS.year, DEC);
//        Serial.print("Fix: ");
//        Serial.print((int)GPS.fix);
//        if (GPS.fix)
//        {
//            Serial.print("Location: ");
//            Serial.print(GPS.latitude, 4);
//            Serial.print(GPS.lat);
//            Serial.print(", ");
//            Serial.print(GPS.longitude, 4);
//            Serial.println(GPS.lon);
//        }
//    
//}

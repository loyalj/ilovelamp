#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>

// All of the lights
#define R_LED D2
#define G_LED D0
#define B_LED D1
#define W_LED D5

// Wifi shit
const char* ssid = "...";
const char* password = "...";
const char* host = "iLoveLamp";

// Setup our web server
ESP8266WebServer server(80);

/**
 * Examines the extension of the file being served and
 * then attaches the correct http header to the response
 */
String getContentType(String filename){
    if(server.hasArg("download")) return "application/octet-stream";
    else if(filename.endsWith(".htm")) return "text/html";
    else if(filename.endsWith(".html")) return "text/html";
    else if(filename.endsWith(".css")) return "text/css";
    else if(filename.endsWith(".js")) return "application/javascript";
    else if(filename.endsWith(".png")) return "image/png";
    else if(filename.endsWith(".gif")) return "image/gif";
    else if(filename.endsWith(".jpg")) return "image/jpeg";
    else if(filename.endsWith(".ico")) return "image/x-icon";
    else if(filename.endsWith(".xml")) return "text/xml";
    else if(filename.endsWith(".pdf")) return "application/x-pdf";
    else if(filename.endsWith(".zip")) return "application/x-zip";
    else if(filename.endsWith(".gz")) return "application/x-gzip";
    return "text/plain";
}

/**
 * Attemps to read from the file system at the given path.
 * If there path is "/", the default index.htm will be used.
 * If the file is not found, a false flag is returned.
 */
bool handleFileRead(String path){

    if(path.endsWith("/")) {
        path += "index.htm";
    }

    String contentType = getContentType(path);
    String pathWithGz = path + ".gz";

    if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
        if(SPIFFS.exists(pathWithGz)) {
            path += ".gz";
        }
        File file = SPIFFS.open(path, "r");
        size_t sent = server.streamFile(file, contentType);
        file.close();
        return true;
    }

    return false;
}

/**
 * Executed when the hardware is initialized, this function
 * initiatis the lights, the file system, and the web server.
 */
void setup() {
    // Setting the lED pins to be output pins
    pinMode(R_LED, OUTPUT); // Red
    pinMode(G_LED, OUTPUT); // Green
    pinMode(B_LED, OUTPUT); // Blue
    pinMode(W_LED, OUTPUT); // White

    // Start files system
    SPIFFS.begin();

    // Attempting to connect to the specified WIFI access point.
    if (String(WiFi.SSID()) != String(ssid)) {
      WiFi.begin(ssid, password);
    }

    // Keep an eye on the connection status while it's
    // being negotiated with the access point.
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    // Get a DNS configuration from the host
    MDNS.begin(host);

    // accept the color in RGB and generate a white value
    server.on("/rgbw", []() {
        String red=server.arg("red");
        int redVal = red.toInt();

        String green=server.arg("green");
        int greenVal = green.toInt();

        String blue=server.arg("blue");
        int blueVal = blue.toInt();

        // Find the minimum rgb value and use that for white
        // http://nouvoyance.com/files/pdf/Adding-a-White.pdf
        // It works.. aight...
        int whiteVal = redVal < greenVal ? redVal : greenVal;
        whiteVal = whiteVal < blueVal ? whiteVal : blueVal;

        // todo: Add a check for a 'white' arg and override the generated value

        engageLightRGBW(redVal, greenVal, blueVal, whiteVal);
        server.send(200, "text/plain", "LED set in RGBW");
    });

    // accept the color in RGB and don't use white
    server.on("/rgb", []() {
        String red=server.arg("red");
        int redVal = red.toInt();

        String green=server.arg("green");
        int greenVal = green.toInt();

        String blue=server.arg("blue");
        int blueVal = blue.toInt();

        engageLightRGB(redVal, greenVal, blueVal);
        server.send(200, "text/plain", "LED set in RGB");
    });

    // Attempt to read the file, if it's not found then 404 out
    server.onNotFound([](){
        if(!handleFileRead(server.uri()))
        server.send(404, "text/plain", "FileNotFound");
    });
    server.begin();
}

/**
 * Executed each cycle on the hardware to keep the server
 * checking for new requests
 */
void loop() {
    server.handleClient();
}

// Color and light things
/**
 * Writes a set of RGBW values to the lights
 */
void engageLightRGB(int red, int green, int blue) {
  engageLightRGBW(red, green, blue, 0);
}

void engageLightRGBW(int red, int green, int blue, int white) {
  // Quick hack fix for the red overpowering everything.
  // This brings the yellows/blues/greens back into line
  // and doesn't seem to affect the reds/pinks too much
  red = (int)((float)red * 0.2f);
  
  analogWrite(R_LED, red);
  analogWrite(G_LED, green);
  analogWrite(B_LED, blue);
  analogWrite(W_LED, white);
}

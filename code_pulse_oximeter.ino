#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30100_PulseOximeter.h"
#include <ESP_Mail_Client.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define REPORTING_PERIOD_MS 1000

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Create a PulseOximeter object
PulseOximeter pox;

SMTPSession smtp;
SMTP_Message message;

const int buttonPin = D3; // Pin connected to the push button

// Wi-Fi credentials
const char* ssid = "PSITPSIT";
const char* password = "1234567890";

// SMTP server details
const char* smtp_server = "smtp.gmail.com";
const int smtp_port = 465;
const char* smtp_user = "ksoham1508@gmail.com";
const char* smtp_password = "xmalpnebusiptutv";

Session_Config session_config;

bool takeReadings = true; // Flag to control whether to take readings

void setupSMTPConfig() {
    session_config.server.host_name = smtp_server;
    session_config.server.port = smtp_port;
    session_config.login.email = smtp_user;
    session_config.login.password = smtp_password;
    session_config.login.user_domain = "gmail.com";
}

// Callback routine executed when a pulse is detected
void onBeatDetected() {
    Serial.println("♥ Beat!");
}

void setup() {
    Serial.begin(115200);

    WiFi.begin(ssid, password);

    Serial.println("Connecting to Wi-Fi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("Connected to Wi-Fi.");

    setupSMTPConfig();

    Serial.print("Initializing pulse oximeter...");

    if (!pox.begin()) {
        Serial.println("FAILED");
        while (true);
    } else {
        Serial.println("SUCCESS");
    }

    pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
    pox.setOnBeatDetectedCallback(onBeatDetected);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("SSD1306 allocation failed");
        while (true);
    }

    display.clearDisplay();

    pinMode(buttonPin, INPUT_PULLUP); // Set up the button pin
}

void sendEmail(float heartRate, float spo2) {

    // Prepare email content
    message.sender.name = "ESP32 Health Monitor";
    message.sender.email = smtp_user;
    message.subject = "Health Monitor Reading";

    String textContent = "Heart rate: " + String(heartRate) + " bpm\nSpO2: " + String(spo2) + "%";
    message.text.content = textContent.c_str();
    message.addRecipient("Recipient Name", "hsharma7985@gmail.com");

    int retryCount = 0;
    const int maxRetries = 3;

    bool emailSent = false;

    while (!emailSent && retryCount < maxRetries) {
        Serial.println("Connecting to SMTP server...");
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("Connecting to SMTP...");
        display.display();

        smtp.connect(&session_config);

        Serial.println("Sending email...");
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("Sending email...");
        display.display();

        if (!MailClient.sendMail(&smtp, &message)) {
            Serial.println("Failed to send email: " + smtp.errorReason());
            display.clearDisplay();
            display.setCursor(0, 0);
            display.print("Failed to send email");
            display.display();
            retryCount++;
        } else {
            Serial.println("Email sent successfully.");
            display.clearDisplay();
            display.setCursor(0, 0);
            display.print("Email sent!");
            display.display();
            emailSent = true;
        }

        smtp.closeSession(); // Close the session to free resources
    }

    // Perform actions after email is sent
    if (emailSent) {
        // Reset flag to resume readings
        takeReadings = true;
    }
}

void loop() {
    if (takeReadings) {
        pox.update();
        float heartRate = pox.getHeartRate();
        float spo2 = pox.getSpO2();

        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.print("Heart rate: ");
        display.print(heartRate);
        display.print(" bpm");
        display.setCursor(0, 10);
        display.print("SpO2: ");
        display.print(spo2);
        display.print("%");
        display.display();
    }

    static unsigned long lastPressTime = 0;
    const unsigned long debounceTime = 200; // Debounce time in milliseconds

    if (digitalRead(buttonPin) == LOW && millis() - lastPressTime >= debounceTime) {
        lastPressTime = millis(); // Update last press time
        sendEmail(pox.getHeartRate(), pox.getSpO2());
    }
}

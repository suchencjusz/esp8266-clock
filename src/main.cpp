#include <Arduino.h>

#include <ArduinoJson.h>
#include <Wire.h>
#include <SPI.h>

#include <NTPClient.h>
#include <TaskScheduler.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>

#include "Adafruit_Si7021.h"
#include "SSD1306Wire.h" // legacy: #include "SSD1306.h"

const char *ssid = "XXXXXX";
const char *password = "";

// const char *weekDays[] = {"Niedziela", "Poniedzialek", "Wtorek", "Sroda", "Czwartek", "Piatek", "Sobota"};
const char *weekDays[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

const String AIRLY_URL = "https://airapi.airly.eu/v2/measurements/installation?installationId=XXXXXXXX";
const String AIRLY_SECRET = "XXXXXXXXXXXXXXXXXXXX";
const String GITHUB_URL = "https://api.github.com/repos/XXXXXXXXXX/YYYYYYYYYY/commits/main";
const String GITHUB_TOKEN = "Bearer XXXXXXXXXXXXXXXXXXXXX";

const time_t timezone_offset = 3600;

SSD1306Wire display(0x3c, 4, 5);
SSD1306Wire display2(0x3d, 4, 5);

WiFiClientSecure client;
HTTPClient http;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "tempus1.gum.gov.pl", 3600, 300000);
Adafruit_Si7021 sensor = Adafruit_Si7021();

#pragma region TASKS
void screenOneUpdateCallback();
void screenTwoUpdateCallback();

void airQualityGetCallback();
void timeCheckCallback();
void temperateureInGetCallback();
void githubRepoGetCallback();
// void weatherCastCallback();

Task screenOneUpdateTask(50, TASK_FOREVER, &screenOneUpdateCallback);
Task screenTwoUpdateTask(150, TASK_FOREVER, &screenTwoUpdateCallback);

Task airQualityGetTask(15 * 60 * 1000, TASK_FOREVER, &airQualityGetCallback);
Task timeCheckTask(10 * 60 * 1000, TASK_FOREVER, &timeCheckCallback);
Task temperateureInGetTask(60 * 1000, TASK_FOREVER, &temperateureInGetCallback);
Task githubRepoGetTask(5 * 60 * 1000, TASK_FOREVER, &githubRepoGetCallback);
// Task weatherCastTask(30 * 60 * 1000, TASK_FOREVER, &weatherCastCallback);

Scheduler runner;
#pragma endregion

float temperatureIn = 0;
float humidityIn = 0;

class AirLyData
{
public:
  float pm1, pm25, pm10, pressure, humidity, temperature, airly_caqi;

  // Default constructor
  AirLyData() : pm1(0), pm25(0), pm10(0), pressure(0), humidity(0), temperature(0), airly_caqi(0) {}
};

class GitHubRepoData
{
public:
  String last_commit_date;
  String last_commit_message;
  String last_commit_author;

  // Default constructor
  GitHubRepoData() : last_commit_date(""), last_commit_message(""), last_commit_author("") {}
};

AirLyData airQuality;
GitHubRepoData gitHubRepo;

void setup()
{
  display.setContrast(20);
  display2.setContrast(100);

  Serial.begin(115200);
  Serial.println();
  display.init();
  display2.init();
  sensor.begin();
  sensor.heater(false);
  sensor.reset();

  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);

  display2.flipScreenVertically();
  display2.setFont(ArialMT_Plain_10);
  display2.setTextAlignment(TEXT_ALIGN_LEFT);

  WiFi.begin(ssid, password);

  int tries = 1;

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    display.drawString(0, 0, "Connecting to WiFi..");
    display.drawProgressBar(0, 14, 120, 10, tries * 10);
    display.display();

    tries++;

    if (tries > 20)
    {
      ESP.restart();
    }
  }

  timeClient.begin();
  display.clear();

  tries = 1;

  while (timeClient.getEpochTime() < 1000000000)
  {
    timeClient.update();
    delay(100);
    display.drawString(0, 0, "Getting time..");
    display.drawProgressBar(0, 14, 120, 10, tries * 10);
    display.display();

    tries++;

    if (tries > 20)
    {
      ESP.restart();
    }
  }

  // Add tasks to the scheduler
  runner.addTask(screenOneUpdateTask);
  runner.addTask(screenTwoUpdateTask);

  runner.addTask(timeCheckTask);
  runner.addTask(temperateureInGetTask);
  runner.addTask(airQualityGetTask);
  runner.addTask(githubRepoGetTask);
  // runner.addTask(weatherCastTask);

  // Enable the tasks
  screenOneUpdateTask.enable();
  screenTwoUpdateTask.enable();

  timeCheckTask.enable();
  temperateureInGetTask.enable();
  airQualityGetTask.enable();
  githubRepoGetTask.enable();
  // weatherCastTask.enable();

  Serial.println("Setup done");
  Serial.println("Setup done");
  Serial.println("Setup done");

  Serial.println("ip address: " + WiFi.localIP().toString());
}

void loop()
{
  runner.execute();
}

void temperateureInGetCallback()
{
  temperatureIn = sensor.readTemperature();
  humidityIn = sensor.readHumidity();
}

void airQualityGetCallback()
{
  client.setInsecure(); // todo: fix for https 

  http.useHTTP10(true);
  http.begin(client, AIRLY_URL);
  http.addHeader("apikey", AIRLY_SECRET);

  int httpCode = http.GET();

  if (httpCode != 200)
  {
    Serial.println(httpCode);
    airQuality.pm1 = -200;
    return;
  }

  // DynamicJsonDocument filter(4096);
  // filter["current"] = true;

  DynamicJsonDocument doc(2048);
  deserializeJson(doc, http.getStream());

  http.end();

  Serial.println(doc["current"]["values"][0]["name"].as<String>() + ": " + doc["current"]["values"][0]["value"].as<String>());
  Serial.println(doc["current"]["values"][1]["name"].as<String>() + ": " + doc["current"]["values"][1]["value"].as<String>());
  Serial.println(doc["current"]["values"][2]["name"].as<String>() + ": " + doc["current"]["values"][2]["value"].as<String>());
  Serial.println(doc["current"]["values"][3]["name"].as<String>() + ": " + doc["current"]["values"][3]["value"].as<String>());
  Serial.println(doc["current"]["values"][4]["name"].as<String>() + ": " + doc["current"]["values"][4]["value"].as<String>());
  Serial.println(doc["current"]["values"][5]["name"].as<String>() + ": " + doc["current"]["values"][5]["value"].as<String>());

  airQuality.pm1 = doc["current"]["values"][0]["value"].as<float>();
  airQuality.pm25 = doc["current"]["values"][1]["value"].as<float>();
  airQuality.pm10 = doc["current"]["values"][2]["value"].as<float>();
  airQuality.pressure = doc["current"]["values"][3]["value"].as<float>();
  airQuality.humidity = doc["current"]["values"][4]["value"].as<float>();
  airQuality.temperature = doc["current"]["values"][5]["value"].as<float>();
  airQuality.airly_caqi = doc["current"]["indexes"][0]["value"].as<float>();
}

void githubRepoGetCallback()
{
  client.setInsecure(); // fix for https

  http.useHTTP10(true);
  http.begin(client, GITHUB_URL);
  http.addHeader("Accept", "application/vnd.github+json");
  http.addHeader("Authorization", GITHUB_TOKEN);

  int httpCode = http.GET();

  if (httpCode != 200)
  {
    Serial.println(httpCode);
    return;
  }

  DynamicJsonDocument doc(2048);
  deserializeJson(doc, http.getStream());

  http.end();

  Serial.println(doc["commit"]["committer"]["date"].as<String>());
  Serial.println(doc["commit"]["message"].as<String>());
  Serial.println(doc["commit"]["committer"]["name"].as<String>());

  gitHubRepo.last_commit_date = doc["commit"]["committer"]["date"].as<String>();
  gitHubRepo.last_commit_message = doc["commit"]["message"].as<String>();
  gitHubRepo.last_commit_author = doc["commit"]["committer"]["name"].as<String>();
}

void timeCheckCallback()
{
  timeClient.update();
}

String convertSecondsToReadableTime(int seconds)
{
  if (seconds < 60)
  {
    return String(seconds) + " s ago";
  }
  else if (seconds < 3600)
  {
    return String(seconds / 60) + " m ago";
  }
  else if (seconds < 86400)
  {
    return String(seconds / 3600) + " h ago";
  }
  else
  {
    return String(seconds / 86400) + " d ago";
  }
}

double ConvertDateStringToPastSecondsFromNow(String date)
{
  int year = date.substring(0, 4).toInt();
  int month = date.substring(5, 7).toInt();
  int day = date.substring(8, 10).toInt();
  int hour = date.substring(11, 13).toInt();
  int minute = date.substring(14, 16).toInt();
  int second = date.substring(17, 19).toInt();

  Serial.println(year);
  Serial.println(month);
  Serial.println(day);
  Serial.println(hour);
  Serial.println(minute);
  Serial.println(second);

  struct tm timeinfo;
  timeinfo.tm_year = year - 1900;
  timeinfo.tm_mon = month - 1;
  timeinfo.tm_mday = day;
  timeinfo.tm_hour = hour;
  timeinfo.tm_min = minute;
  timeinfo.tm_sec = second;

  time_t time = mktime(&timeinfo);
  time_t now = timeClient.getEpochTime();

  time += timezone_offset;

  Serial.println(difftime(now, time));

  return difftime(now, time);
}

int textXPos = 128;

void screenOneUpdateCallback()
{
  display.clear();

  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);

  int lastCommitedSecondsAgo = ConvertDateStringToPastSecondsFromNow(gitHubRepo.last_commit_date);
  String lastCommitedReadableTimeAgo = convertSecondsToReadableTime(lastCommitedSecondsAgo) + "·" + gitHubRepo.last_commit_author + "·" + gitHubRepo.last_commit_message;

  // Calculate the width of the text
  int textWidth = lastCommitedReadableTimeAgo.length() * 6;

  // Draw the text at the current x-position
  display.drawString(textXPos, 0, lastCommitedReadableTimeAgo);

  // Decrease the x-position for the next frame
  textXPos--;

  // If the text has moved off the screen, reset the x-position
  if (textXPos < -textWidth)
  {
    textXPos = 128;
  }

  String temperatureInText = String(temperatureIn) + " C";
  display.drawString(0, 54, temperatureInText);

  String humidityInText = String(humidityIn) + " %";
  display.drawString(128 - display.getStringWidth(humidityInText), 54, humidityInText);

  display.setFont(ArialMT_Plain_24);

  String timeText = timeClient.getFormattedTime();
  int16_t timeTextWidth = display.getStringWidth(timeText);
  int16_t timeTextHeight = 24;
  int16_t centerX = (display.getWidth() - timeTextWidth) / 2;
  int16_t centerY = (display.getHeight() - timeTextHeight) / 2 - 5;
  display.drawString(centerX, centerY, timeText);

  display.setFont(ArialMT_Plain_10);

  String weekDayText = weekDays[timeClient.getDay()];
  int16_t weekDayTextWidth = display.getStringWidth(weekDayText);
  int16_t weekDayCenterX = (display.getWidth() - weekDayTextWidth) / 2;
  int16_t weekDayCenterY = centerY + timeTextHeight;
  display.drawString(weekDayCenterX, weekDayCenterY, weekDayText);

  display.display();
}

void screenTwoUpdateCallback()
{
  display2.clear();

  display2.setFont(ArialMT_Plain_10);

  String pm1Text = String(airQuality.pm1) + " PM1";
  display2.drawString(0, 0, pm1Text);

  String pm25Text = String(airQuality.pm25) + " PM2.5";
  display2.drawString(0, 10, pm25Text);

  String pm10Text = String(airQuality.pm10) + " PM10";
  display2.drawString(0, 20, pm10Text);

  String pressureText = String(airQuality.pressure) + " hPa";
  display2.drawString(0, 30, pressureText);

  String humidityText = String(airQuality.humidity) + " %";
  display2.drawString(0, 40, humidityText);

  String temperatureText = String(airQuality.temperature) + " C";
  display2.drawString(0, 50, temperatureText);

  // DISPLAY AIRLY_CAQITEXT ON RIGHT SIDE OF SCREEN CENTERED
  display2.setFont(ArialMT_Plain_24);

  String airly_caqiText = String(int(airQuality.airly_caqi));

  int16_t airly_caqiTextWidth = display2.getStringWidth(airly_caqiText);
  int16_t airly_caqiTextHeight = 24; 
  int16_t airly_caqiCenterX = (display2.getWidth() - airly_caqiTextWidth) / 2 + 32;
  int16_t airly_caqiCenterY = (display2.getHeight() - airly_caqiTextHeight) / 2;
  display2.drawString(airly_caqiCenterX, airly_caqiCenterY, airly_caqiText);

  display2.display();
}
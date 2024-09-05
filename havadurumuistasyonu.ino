#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <WiFi.h>
#include <Wire.h>
#include <Deneyap_BasincOlcer.h>
#include <Deneyap_SicaklikNemBasincOlcer.h>
#include <HTTPClient.h>

AtmosphericPressure Pressure;
SHT4x TempHum;

float basinc;
float sicaklik;
float nem;

const char* ssid = "WIFI-İSMİNİZ";
const char* password = "WIFI-ŞİFRENİZ";

const char* telegramBotToken = "TELEGRAMBOT-TOKENINIZ";
const int telegramChatIds[] = {SİZİN-TELEGRAM-CHAT-ID'NİZ, ARKADAŞINIZIN-TELEGRAM-CHAT-ID'Sİ"(GEREKLİ DEĞİL)"};
const int numChatIds = sizeof(telegramChatIds) / sizeof(telegramChatIds[0]);

WiFiServer server(80);

// HTTP isteklerini ve yanıt zamanlamalarını saklamak için değişkenler
String header;
unsigned long currentTime = millis();
unsigned long previousHttpResponseTime = 0;
unsigned long previousTelegramMessageTime = 0;
const long timeoutTime = 2000;
const long telegramMessageInterval = 10000;

void setup() {
  // Hata ayıklama için seri iletişimi başlatma
  Serial.begin(115200);
  
  // Sensörleri başlatma (basınç ve sıcaklık/nem)
  Pressure.begin(0x76);
  if (!TempHum.begin(0X44)) {
    Serial.println("I²C bağlantısı başarısız ");
    while (1);
  }

  // Wi-Fi ağına bağlanma
  Serial.print("Wi-Fi ağına bağlanılıyor: ");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi bağlantısı başarılı.");
  Serial.print("IP Adresi: ");
  Serial.println(WiFi.localIP());

  // Gelen HTTP isteklerini dinlemek için sunucuyu başlatın
  server.begin();
}

void loop() {
  // Bir istemcinin bağlanmasını bekleyin
  WiFiClient client = server.available();

  // Sensörlerden sıcaklık, nem ve basınç ölçme
  TempHum.measure();
  sicaklik = TempHum.TtoDegC();
  nem = TempHum.RHtoPercent();
  basinc = Pressure.getPressure();

  // Bir istemci bağlanırsa, HTTP isteğini ve yanıtını işleyin
  if (client) {
    currentTime = millis();
    previousHttpResponseTime = currentTime;
    Serial.println("New Client.");
    String currentLine = "";
    
    // İstemcinin HTTP isteğini okuyun
    while (client.connected() && currentTime - previousHttpResponseTime <= timeoutTime) {
      currentTime = millis();
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        header += c;
        
        // İstemcinin isteğinin sonunu kontrol et
        if (c == '\n') {
          if (currentLine.length() == 0) {
            // Send the HTTP response with sensor data to the client
            sendHttpResponse(client);
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    header = "";
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }

  // Tanımlanan aralıklarla Telegram'a veri gönderme
  if (currentTime - previousTelegramMessageTime >= telegramMessageInterval) {
    sendTelegramMessage();
    previousTelegramMessageTime = currentTime;
  }
}

void sendHttpResponse(WiFiClient client) {
  // Standart HTTP başlıklarını gönder
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println("Refresh: 5");
  client.println();
  
  // Sensör verilerini bir web sayfasında görüntülemek için HTML yanıtı oluşturun
  client.println("<!DOCTYPE html>");
  client.println("<html lang=\"en\">");
  client.println("<head>");
  client.println("    <meta charset=\"UTF-8\">");
  client.println("    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">");
  client.println("    <title>Hava Durumu İstasyonu</title>");
  client.println("    <link href=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/css/bootstrap.min.css\" rel=\"stylesheet\" integrity=\"sha384-QWTKZyjpPEjISv5WaRU9OFeRpok6YctnYmDr5pNlyT2bRjXh0JMhjY6hW+ALEwIH\" crossorigin=\"anonymous\">");
  client.println("    <link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.5.2/css/all.min.css\" integrity=\"sha512-SnH5WK+bZxgPHs44uWIX+LLJAJ9/2PkPKZ5QiAj6Ta86w+fsb2TkcmfRyVX3pBnMFcV7oQPJkl9QevSCWr3W6A==\" crossorigin=\"anonymous\" referrerpolicy=\"no-referrer\" />");
  client.println("    <style>");
  client.println("        body {");
  client.println("            margin: 0;");
  client.println("            box-sizing: border-box;");
  client.println("            min-height: 100vh;");
  client.println("            display: flex;");
  client.println("            justify-content: center;");
  client.println("            align-items: center; ");
  client.println("            background: linear-gradient(245.59deg,#34e89e 0% ,#0f3443 28.53% , #000000 75.52%);");
  client.println("        }");
  client.println("        .card {");
  client.println("            background: transparent;");
  client.println("            border: 2px solid;");
  client.println("            border-image: linear-gradient(45deg, #ff8a00, #e52e71) 1;");
  client.println("            border-radius: 8px;");
  client.println("            width: 18rem;");
  client.println("        }");
  client.println("        i {");
  client.println("            font-size: 70px;");
  client.println("        }");
  client.println("        .card-text, i, .card-title, h1, h2, span {");
  client.println("            background: linear-gradient(45deg, #ff8a00, #e52e71);");
  client.println("            -webkit-background-clip: text;");
  client.println("            color: transparent;");
  client.println("        }");
  client.println("    </style>");
  client.println("</head>");
  client.println("<body>");
  
  // Sıcaklık (Celsius ve Fahrenheit), basınç ve nemi görüntüleme
  client.println("    <div class=\"container\">");
  client.println("        <div class=\"row\">");
  client.println("            <h1 class=\"text-center mb-2 p-3\">HAVA DURUMU İSTASYONU</h1>");
  client.println("        </div>");
  client.println("        <div class=\"row text-center justify-content-center\">");
  client.println("            <div class=\"card col-md-3 col-sm-6 mx-3 my-2 d-flex justify-content-center\" >");
  client.println("                <i class=\"fa-solid fa-temperature-low d-flex justify-content-center align-items-center p-4\"></i>");
  client.println("                <div class=\"card-body\">");
  client.println("                    <h4 class=\"card-title text-center p-3\">SICAKLIK</h4>");
  client.println("                    <h2 class=\"card-text p-4 text-center\">" + String(sicaklik) + " C°</h2>");
  client.println("                </div>");
  client.println("            </div>");
  client.println("            <div class=\"card col-md-3 col-sm-6 mx-3 my-2\">");
  client.println("                <i class=\"fa-solid fa-temperature-quarter d-flex justify-content-center align-items-center p-4\"><span style=\"font-size: 15px; position: absolute;top: 10%; left: 60%;\">F°</span></i>");
  client.println("                <div class=\"card-body\">");
  client.println("                    <h4 class=\"card-title text-center p-3\">SICAKLIK</h4>");
  client.println("                    <h2 class=\"card-text p-4 text-center\">" + String((sicaklik * 1.8) + 32) + " F°</h2>");
  client.println("                </div>");
  client.println("            </div>");
  client.println("            <div class=\"card col-md-3 col-sm-6 mx-3 my-2\">");
  client.println("                <i class=\"fa-solid fa-gauge d-flex justify-content-center align-items-center p-4\"></i>");
  client.println("                <div class=\"card-body\">");
  client.println("                    <h4 class=\"card-title text-center p-3\">BASINÇ</h4>");
  client.println("                    <h2 class=\"card-text p-4 text-center\">" + String(basinc) + " hPa</h2>");
  client.println("                </div>");
  client.println("            </div>");
  client.println("            <div class=\"card col-md-3 col-sm-6 mx-3 my-2\">");
  client.println("                <i class=\"fa-solid fa-droplet d-flex justify-content-center align-items-center p-4\"></i>");
  client.println("                <div class=\"card-body\">");
  client.println("                    <h4 class=\"card-title text-center p-3\">NEM</h4>");
  client.println("                    <h2 class=\"card-text p-4 text-center\">" + String(nem) + " %</h2>");
  client.println("                </div>");
  client.println("            </div>");
  client.println("        </div>");
  client.println("    </div> ");
  client.println("</body>");
  client.println("</html>");
}

void sendTelegramMessage() {
  // Sensör verilerini tanımlanmış tüm Telegram sohbet kimliklerine gönderin
  for (int i = 0; i < numChatIds; i++) {
    DynamicJsonDocument json(200);
    
    // JSON nesnesini sensör verileri ve sohbet kimliği ile doldurun
    json["chat_id"] = telegramChatIds[i];
    json["text"] = "Sıcaklık (C°): " + String(sicaklik) + " C°\n" +
                   "Sıcaklık (F°): " + String((sicaklik * 1.8) + 32) + " F°\n" +
                   "Nem: " + String(nem) + " %\n" +
                   "Basınç: " + String(basinc) + " hPa";

    // JSON nesnesini bir dizeye serileştirme
    String jsonString;
    serializeJson(json, jsonString);

    // Telegram API'sine bir HTTP POST isteği gönderin
    HTTPClient http;
    http.begin("https://api.telegram.org/bot" + String(telegramBotToken) + "/sendMessage");
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(jsonString);
    if (httpResponseCode > 0) {
      Serial.print("Telegram message sent successfully to chat id ");
      Serial.print(telegramChatIds[i]);
      Serial.print(". Response code: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Error sending Telegram message to chat id ");
      Serial.print(telegramChatIds[i]);
      Serial.print(". HTTP error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
}
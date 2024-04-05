


#include <WiFi.h>

const char* ssid = "kifli";
const char* password = "0987654098";
const int ledPin = 2;
const int buttonPin = 15;
int frequency = 10; // Frekuensi base
int saldoAwal = 740000; // Saldo awal dalam rupiah
int nilaiTransaksi = 20000; // Nilai nominal transaksi

WiFiServer server(80);

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("connected");
  Serial.println("IP Address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void tampilkanPesan(WiFiClient &client, const String &pesan, bool transaksiBerhasil) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();
  client.println("<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><style>.button {background-color: #007bff;border: none;color: white;padding: 16px 18px;text-align: center;text-decoration: none;display: inline-block;font-size: 16px;margin: 4px 2px;transition-duration: 0.4s;cursor: pointer;border-radius: 50%;}.button:hover {background-color: #0056b3;}.container {text-align: center;margin-top: 50px;}h1 {font-size: 24px;}h2 {font-size: 20px;}</style></head><body>");
  client.println("<div class=\"container\">");
  client.println("<h1>Saldo: Rp. " + String(saldoAwal) + "</h1>");
  if (!transaksiBerhasil) {
    client.println("<form action='/topup' method='POST'>");
    client.println("<label for='jumlah'>Jumlah Top Up:</label>");
    client.println("<input type='number' id='jumlah' name='jumlah' min='0'><br><br>");
    client.println("<input type='submit' value='Top Up'>");
    client.println("</form>");
  }
  client.println("<button class=\"button\" onclick=\"location.href='/transaksi'\">Transaksi</button>");
  client.println(pesan);
  client.println("</div></body></html>");
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    String currentLine = "";
    String postData = "";
    boolean blankLineFound = false;
    boolean transaksiBerhasil = false; // Menyimpan status transaksi berhasil atau tidak
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        if (c == '\n') {
          if (currentLine.length() == 0) {
            tampilkanPesan(client, "", transaksiBerhasil); // Mengirim status transaksi berhasil ke fungsi tampilkanPesan
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
        // Baca data dari badan permintaan POST
        if (currentLine.startsWith("POST /topup")) {
          blankLineFound = true;
        } else if (blankLineFound) {
          postData += c;
        }
      }
      if (blankLineFound) {
        // Proses data POST setelah baris kosong
        if (postData.length() > 0) {
          // Parsing data POST untuk mendapatkan jumlah saldo
          int pos = postData.indexOf("jumlah=");
          if (pos != -1) {
            String requestJumlah = postData.substring(pos + 7); // 7 adalah panjang string "jumlah="
            saldoAwal += requestJumlah.toInt();
            tampilkanPesan(client, "Saldo berhasil ditambahkan sebesar Rp. " + requestJumlah, false);
          } else {
            tampilkanPesan(client, "Mohon masukkan jumlah saldo yang valid.", false);
          }
        } else {
          tampilkanPesan(client, "Permintaan top up tidak valid.", false);
        }
        break;
      }
      if (currentLine.indexOf("GET /transaksi") != -1) {
        if (saldoAwal >= nilaiTransaksi) {
          digitalWrite(ledPin, HIGH); // LED menyala
          delay(5000); // LED menyala selama 5 detik
          saldoAwal -= nilaiTransaksi;
          transaksiBerhasil = true;
          digitalWrite(ledPin, LOW); // Matikan LED setelah transaksi berhasil
          tampilkanPesan(client, "TRANSAKSI BERHASIL, SISA SALDO Rp. " + String(saldoAwal), true); // Mengirim status transaksi berhasil ke fungsi tampilkanPesan
        } else {
          for (int i = 0; i < 10; i++) {
            digitalWrite(ledPin, HIGH);
            delay(250);
            digitalWrite(ledPin, LOW);
            delay(250);
          }
          tampilkanPesan(client, "SALDO TIDAK MENCUKUPI", false);
        }
      }
    }
    client.stop();
  }
}


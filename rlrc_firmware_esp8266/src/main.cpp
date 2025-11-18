#include <Arduino.h>
#include <ESP8266WiFi.h>

// --- 1. WiFi 配置 ---
// !!! 修改为你自己的 WiFi !!!
const char* ssid = "Meizu 20 Pro";
const char* password = "9876543210";

// --- 2. TCP 服务器配置 ---
constexpr uint16_t TCP_PORT = 8080;
WiFiServer tcpServer(TCP_PORT); // 创建 TCP 服务器对象
WiFiClient tcpClient;          // 创建一个全局客户端对象

// --- 3. 设置 (Setup) ---
void setup() {
  // 启动 串口 (Serial)，用于转发数据给 STM32
  // 并且也用于在 "Serial Monitor" 中打印调试信息
  Serial.begin(115200);
  Serial.println("\n\rESP8266 WiFi-to-UART Bridge");

  // --- 连接 WiFi ---
  Serial.printf("Connecting to %s...", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n\rWiFi Connected!");

  // --- 启动 TCP 服务器 ---
  tcpServer.begin();
  Serial.printf("TCP Server started on port %d\n\r", TCP_PORT);

  // --- 关键：打印自己的 IP 地址 ---
  // (你需要把这个 IP 告诉 Flutter APP)
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// --- 4. 主循环 (Loop) ---
void loop() {
  // --- 检查是否有新的 APP 连接 ---
  if (!tcpClient.connected()) {
    // 如果没有客户端连接，则尝试接受一个新连接
    tcpClient = tcpServer.accept();
    if (tcpClient) {
      Serial.println("New client connected!");
    }
  }

  // 如果客户端已连接...
  if (tcpClient.connected()) {

    // --- 检查 APP (TCP) 是否发来数据 ---
    while (tcpClient.available()) {
      // 从 TCP 读取数据
      uint8_t data = tcpClient.read();

      // !!! 核心网桥逻辑 !!!
      // 将数据原样通过 串口 (Serial) 转发给 STM32
      Serial.write(data);
    }

    // (可选) 检查 STM32 是否发来数据，并发回 APP
    // while (Serial.available()) {
    //    uint8_t data = Serial.read();
    //    tcpClient.write(data);
    // }
  }
}
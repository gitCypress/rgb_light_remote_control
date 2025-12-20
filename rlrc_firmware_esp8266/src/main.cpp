#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <WiFiUdp.h>

#include "Log.hpp"
#include "WiFiManager.h"
#include "config.hpp"

// TCP server & client 对象
WiFiServer tcpServer(config::tcpPort);
WiFiClient tcpClient;

// UDP 对象
WiFiUDP udp;

// --- 3. 设置 (Setup) ---
void setup() {
    // 串口
    // 1. 转发数据给 STM32
    // 2. 测试时在 Serial Monitor 中打印调试信息
    Serial.begin(config::serialBaudRate);

    delay(2000);
    Log::println("ESP8266 WiFi-to-UART Bridge");

    Ticker ticker;
    WiFiManager wm;
    wm.setDebugOutput(false);
    wm.setConfigPortalTimeout(config::wmPortalTimeout); // AP 门户启动后的配置超时时间
    wm.setAPCallback([&](WiFiManager *myWiFiManager) {
        Log::println("Entered Config Mode.");
        Log::printf("AP SSID: %s\n", myWiFiManager->getConfigPortalSSID().c_str());
        Log::println("IP: 192.168.4.1");

        // 开始闪烁 LED (每 500ms 一次)
        ticker.attach_ms(500, [] {
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        });
    });

    // 自动连接
    if (!wm.autoConnect("AutoConnectAP_RLRC_ESP8266EX")) {
        // 配置超时或失败时
        Log::println("Failed to connect and hit timeout. Rebooting...");
        delay(3000);
        EspClass::restart(); // 重启
        delay(5000);
    }

    ticker.detach(); // 停止闪烁
    digitalWrite(LED_BUILTIN, HIGH); // LED 常灭 (NodeMCU 是低电平点亮)

    Log::println("WiFi Connected!");

    // 启动 TCP 服务器
    tcpServer.begin();
    Log::printf("TCP Server started on port %d", config::tcpPort);

    // 打印自己的 IP 地址
    Log::print("IP Address: ");
    Log::println(WiFi.localIP().toString());

    // 启动 UDP 监听服务
    udp.begin(config::udpPort);
    Log::printf("UDP Discovery listening on port %d\n", config::udpPort);
}

void loop() {
    // 检查是否有新的 APP 连接
    if (!tcpClient.connected()) {
        // 如果没有客户端连接，则尝试接受一个新连接
        tcpClient = tcpServer.accept();
        if (tcpClient) {
            Log::println("New client connected!");
        }
    }

    // 如果客户端已连接
    if (tcpClient.connected()) {

        // 检查 APP 是否发来数据
        while (tcpClient.available()) {
            // 从 TCP 读取数据
            // 这里 ESP32 充当一个透明网桥，它不关心从 TCP 获取的流数据如何截止
            // 只负责将数据原模原样通过串口转发给 STM32
            Serial.write(tcpClient.read());
        }
    }

    // --- UDP 设备发现请求广播处理---
    if (udp.parsePacket()) {
        // 缓冲区接收数据
        char packetBuffer[255];

        if (const int len = udp.read(packetBuffer, 255); len > 0) {
            packetBuffer[len] = 0; // 手动添加字符串结束符，确保安全
        }

        // 校验暗号
        if (String(packetBuffer) == config::discoveryMsg) {
            // 发送回复
            udp.beginPacket(udp.remoteIP(), udp.remotePort());
            udp.print(WiFi.localIP()); // 回复 IP 地址
            udp.endPacket();

        } else {
            // Log::printf("Ignored unknown UDP packet: %s\n", packetBuffer);
        }
    }
}

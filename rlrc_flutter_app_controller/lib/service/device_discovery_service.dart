import 'dart:async';
import 'dart:convert';
import 'dart:io';

class DeviceDiscoveryService {
  static const int udpPort = 8888; // 必须和 ESP8266 config.hpp 里的一致
  static const String discoveryMsg = "WHO_IS_RLRC?"; // 暗号

  // 你的 ESP8266 回复的消息前缀，或者是直接回复 IP
  // 如果 ESP8266 代码是 udp.print(WiFi.localIP())，那收到就是纯 IP 字符串

  /// 扫描设备
  /// [timeout] 扫描持续时间
  /// 返回一个 Stream，每发现一个设备就吐出一个 IP 字符串
  static Stream<String> scanDevices({Duration timeout = const Duration(seconds: 3)}) async* {
    RawDatagramSocket? socket;

    try {
      // 1. 绑定随机端口
      socket = await RawDatagramSocket.bind(InternetAddress.anyIPv4, 0);
      socket.broadcastEnabled = true; // 开启广播权限

      // 2. 发送广播包
      final data = utf8.encode(discoveryMsg);
      // 发送到 255.255.255.255
      socket.send(data, InternetAddress('255.255.255.255'), udpPort);
      print("[Discovery] Broadcast sent...");

      // 3. 监听回复
      // 使用 await for 循环来处理 Stream 事件
      final endTime = DateTime.now().add(timeout);

      await for (RawSocketEvent event in socket) {
        if (DateTime.now().isAfter(endTime)) break;

        if (event == RawSocketEvent.read) {
          final datagram = socket.receive();
          if (datagram != null) {
            final msg = utf8.decode(datagram.data).trim();
            final senderIp = datagram.address.address;

            print("[Discovery] Response from $senderIp: $msg");

            // 简单的过滤逻辑：如果回复的是 IP 格式，或者包含特定标识
            // 这里假设 ESP8266 直接回复了它的 IP 字符串，或者我们只认 IP
            yield senderIp;
          }
        }
      }
    } catch (e) {
      print("[Discovery] Error: $e");
    } finally {
      socket?.close();
    }
  }
}

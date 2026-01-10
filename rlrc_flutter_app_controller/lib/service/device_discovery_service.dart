import 'dart:async';
import 'dart:convert';
import 'dart:io';

class DeviceDiscoveryService {
  static const int udpPort = 8888; // 必须和 ESP8266 config.hpp 里的一致
  static const String discoveryMsg = "WHO_IS_RLRC?"; // 暗号

  // 你的 ESP8266 回复的消息前缀，或者是直接回复 IP
  // 如果 ESP8266 代码是 udp.print(WiFi.localIP())，那收到就是纯 IP 字符串

  /// 获取本地网络的广播地址
  /// 例如：如果本机IP是 192.168.1.100/24，返回 192.168.1.255
  static Future<List<String>> _getBroadcastAddresses() async {
    final broadcastAddresses = <String>[];

    try {
      final interfaces = await NetworkInterface.list();
      for (var interface in interfaces) {
        for (var addr in interface.addresses) {
          if (addr.type == InternetAddressType.IPv4 && !addr.isLoopback) {
            // 计算广播地址
            // 假设都是 /24 子网 (255.255.255.0)
            final parts = addr.address.split('.');
            if (parts.length == 4) {
              final broadcast = '${parts[0]}.${parts[1]}.${parts[2]}.255';
              broadcastAddresses.add(broadcast);
              print("[Discovery] Found network interface: ${addr.address}, broadcast: $broadcast");
            }
          }
        }
      }
    } catch (e) {
      print("[Discovery] Error getting network interfaces: $e");
    }

    // 如果没有找到任何网络接口，添加默认的全局广播地址
    if (broadcastAddresses.isEmpty) {
      broadcastAddresses.add('255.255.255.255');
    }

    return broadcastAddresses;
  }

  /// 扫描设备
  /// [timeout] 扫描持续时间
  /// 返回一个 Stream，每发现一个设备就吐出一个 IP 字符串
  static Stream<String> scanDevices({Duration timeout = const Duration(seconds: 3)}) async* {
    RawDatagramSocket? socket;
    final Set<String> discoveredDevices = {}; // 防止重复

    try {
      // 1. 绑定随机端口
      socket = await RawDatagramSocket.bind(InternetAddress.anyIPv4, 0);
      socket.broadcastEnabled = true; // 开启广播权限

      // 2. 获取所有可能的广播地址
      final broadcastAddresses = await _getBroadcastAddresses();

      // 3. 发送广播包到所有网络接口
      final data = utf8.encode(discoveryMsg);
      for (var broadcastAddr in broadcastAddresses) {
        try {
          socket.send(data, InternetAddress(broadcastAddr), udpPort);
          print("[Discovery] Broadcast sent to $broadcastAddr:$udpPort");
        } catch (e) {
          print("[Discovery] Failed to send to $broadcastAddr: $e");
        }
      }

      // 4. 监听回复
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
            // 使用 Set 防止重复发送同一设备
            if (!discoveredDevices.contains(senderIp)) {
              discoveredDevices.add(senderIp);
              yield senderIp;
            }
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

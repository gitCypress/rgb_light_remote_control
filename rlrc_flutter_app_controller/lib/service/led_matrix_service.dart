import 'dart:io';
import 'dart:typed_data';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:light_controller/service/protocol_encoder.dart';

/// 全局 Provider，让 APP 的任何地方都能访问到这个服务单例
final ledMatrixServiceProvider = Provider<LedMatrixService>((ref) {
  return LedMatrixService(ProtocolEncoder());
});

/// 服务层：负责原始 TCP Socket 的生命周期管理
class LedMatrixService {
  LedMatrixService(this._encoder);

  final ProtocolEncoder _encoder;
  Socket? _socket;

  /// 连接到 ESP8266
  /// (生产级应用会在这里处理 DNS 解析，但我们直接用 IP)
  Future<void> connect(String ip, int port) async {
    try {
      // 如果已连接，先断开
      await disconnect();

      // 连接到 ESP8266 的 TCP 服务器
      _socket = await Socket.connect(
        ip,
        port,
        // 设置一个合理的超时，防止 UI 卡死
        timeout: const Duration(seconds: 5),
      );

      // (可选) 设置 TCP_NODELAY (Kagle 算法)
      // 对于我们这种小数据包，禁用 Nagle 算法可以降低延迟
      _socket?.setOption(SocketOption.tcpNoDelay, true);

      // (可选) 监听来自 STM32/ESP8266 的*返回*数据
      // _socket?.listen(
      //   (data) {
      //     print('Received from STM32: $data');
      //   },
      //   onError: (error) {
      //     print('Socket Error: $error');
      //     disconnect();
      //   },
      //   onDone: () {
      //     print('Socket disconnected by remote.');
      //     disconnect();
      //   },
      // );
    } catch (e) {
      // 确保在出错时 socket 是 null
      _socket = null;
      // 将错误抛出，让状态层 (State Layer) 去处理
      rethrow;
    }
  }

  /// 断开连接
  Future<void> disconnect() async {
    await _socket?.flush(); // 确保所有数据已发送
    await _socket?.close();
    _socket = null;
  }

  /// [私有] 原始的发送方法
  /// cobs 编码将在这里进行
  void _send(Uint8List data) {
    if (_socket == null) {
      print("Socket not connected. Can't send data.");
      // 生产级应用会在这里触发一个“重连”或“错误”状态
      return;
    }
    try {
      final encodedData = _encoder.applyCobs(data);  // cobs
      _socket?.add(encodedData);
    } catch (e) {
      print("Socket send error: $e");
    }
  }

  // --- 公共 API：给状态层调用的“意图” ---

  /// 意图：发送一个“开/关”指令
  void sendToggleCommand(bool isOn) {
    final data = _encoder.encodeBasicToggle(isOn: isOn);
    _send(data);
  }

  /// 意图：发送一个“设置像素”指令
  void sendSetPixelCommand(int x, int y, int r, int g, int b) {
    final data = _encoder.encodeSetPixel(x: x, y: y, r: r, g: g, b: b);
    _send(data);
  }

  /// 意图：发送全屏颜色数据 (画板同步)
  /// [colors] 必须是长度为 25 的 List<Color> (对应 5x5)
  /// 或者是已经处理好的 Uint8List (长度 75)
  void sendFullFrame(List<int> pixelBytes) {
    // 1. 安全检查：确保数据长度正确 (5*5*3 = 75)
    if (pixelBytes.length != 75) {
      print("Error: Frame data length must be 75 bytes (5x5x3).");
      return;
    }

    // 2. 转换为 Uint8List
    final data = Uint8List.fromList(pixelBytes);

    // 3. 编码并发送
    final packet = _encoder.encodeFullFrame(data);
    _send(packet);
  }
}
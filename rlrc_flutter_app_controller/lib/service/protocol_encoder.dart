import 'dart:typed_data';

/// 负责将 APP 的意图 (Intent) 转换为 STM32 能识别的二进制数据包
class ProtocolEncoder {

  /// 指令 4: 开/关 (0x03) (2 字节)
  /// [CMD(0x03)] [STATE(0x00/0x01)]
  Uint8List encodeBasicToggle({required bool isOn}) {
    final builder = BytesBuilder();

    builder.addByte(0x03); // Command ID
    builder.addByte(isOn ? 0x01 : 0x00); // State
    
    return builder.toBytes();
  }

  /// 指令 2: 设置单个像素 (0x01) (6 字节)
  /// [CMD(0x01)] [X] [Y] [R] [G] [B]
  Uint8List encodeSetPixel({
    required int x,
    required int y,
    required int r,
    required int g,
    required int b,
  }) {
    final builder = BytesBuilder();

    builder.addByte(0x01); // Command ID
    builder.addByte(x.clamp(0, 4)); // X
    builder.addByte(y.clamp(0, 4)); // Y
    builder.addByte(r.clamp(0, 255)); // R
    builder.addByte(g.clamp(0, 255)); // G
    builder.addByte(b.clamp(0, 255)); // B

    return builder.toBytes();
  }

  /// 指令 3: 刷新全屏 (0x02) (76 字节)
  /// [CMD(0x02)] [R0,G0,B0, R1,G1,B1, ...]
  /// data 应该是 75 字节的 Uint8List (25 * 3)
  Uint8List encodeFullFrame(Uint8List data) {
    final builder = BytesBuilder();

    builder.addByte(0x02); // Command ID
    builder.add(data); // 75 字节的颜色数据

    return builder.toBytes();
  }
}
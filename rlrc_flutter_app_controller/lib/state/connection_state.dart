import 'package:freezed_annotation/freezed_annotation.dart';

part 'connection_state.freezed.dart'; // 告诉 build_runner 这是它要生成的文件

@freezed
sealed class ConnectionState with _$ConnectionState {

  /// 初始状态，或用户主动断开
  const factory ConnectionState.initial() = _Initial;

  /// 正在连接中
  const factory ConnectionState.connecting() = _Connecting;

  /// 已连接
  const factory ConnectionState.connected() = _Connected;

  /// 发生错误
  const factory ConnectionState.error(String message) = _Error;
}
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../service/led_matrix_service.dart';
import '../state/connection_state.dart';

// 1. 全局 Provider，让 UI 可以 "watch" 和 "read"
final connectionProvider =
    StateNotifierProvider<ConnectionController, ConnectionState>((ref) {
  // 注入服务层
  final service = ref.watch(ledMatrixServiceProvider);
  return ConnectionController(service);
});

// 2. 状态控制器
class ConnectionController extends StateNotifier<ConnectionState> {
  ConnectionController(this._service) : super(const ConnectionState.initial());

  final LedMatrixService _service;

  /// 意图：连接
  Future<void> connect(String ip, int port) async {
    // 立即更新 UI 为“加载中”
    state = const ConnectionState.connecting();

    try {
      await _service.connect(ip, port);
      // 成功
      state = const ConnectionState.connected();
    } catch (e) {
      // 失败 (包括我们设置的 5 秒超时)
      state = ConnectionState.error(e.toString());
    }
  }

  /// 意图：断开连接
  Future<void> disconnect() async {
    await _service.disconnect();
    state = const ConnectionState.initial();
  }
}
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../controllers/connection_controller.dart';
import '../service/led_matrix_service.dart';

// 1. 我们的 UI 是一个 ConsumerWidget
class HomePage extends ConsumerStatefulWidget {
  const HomePage({super.key});

  @override
  ConsumerState<HomePage> createState() => _HomePageState();
}

class _HomePageState extends ConsumerState<HomePage> {
  // 2. 为 IP 地址创建一个 Controller，并预填我们的 IP
  late final TextEditingController _ipController;

  @override
  void initState() {
    super.initState();
    _ipController = TextEditingController(text: '192.168.57.179');
  }

  @override
  void dispose() {
    _ipController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    // 3. "watch" 连接状态，当它改变时，UI 会自动重建
    final connectionState = ref.watch(connectionProvider);

    return Scaffold(
      appBar: AppBar(
        title: const Text('STM32 LED 矩阵控制器'),
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
      ),
      body: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          children: [
            // --- 1. IP 和端口输入 ---
            TextField(
              controller: _ipController,
              decoration: const InputDecoration(
                labelText: 'ESP8266 IP 地址',
                border: OutlineInputBorder(),
              ),
              // 当正在连接或已连接时，禁用输入框
              enabled: connectionState.maybeWhen(
                connecting: () => false,
                connected: () => false,
                orElse: () => true,
              ),
            ),
            const SizedBox(height: 16),

            // --- 2. 动态连接按钮 (核心) ---
            // 我们用 .when() 来根据状态显示不同的 UI
            connectionState.when(
              // 初始状态: 显示“连接”
              initial: () => FilledButton(
                onPressed: _connect,
                child: const Text('连接到 192.168.57.179:8080'),
              ),
              // 连接中: 显示“加载中”
              connecting: () =>
                  const Center(child: CircularProgressIndicator()),
              // 已连接: 显示“断开”
              connected: () => FilledButton.tonal(
                onPressed: _disconnect,
                child: const Text('已连接 (点击断开)'),
              ),
              // 错误: 显示错误和“重试”
              error: (message) => Column(
                children: [
                  Text(
                    '连接失败: $message',
                    style: TextStyle(
                      color: Theme.of(context).colorScheme.error,
                    ),
                  ),
                  const SizedBox(height: 8),
                  FilledButton(onPressed: _connect, child: const Text('重试')),
                ],
              ),
            ),

            // --- 3. 仅在“已连接”时才显示的控制按钮 ---
            if (connectionState.maybeWhen(
              connected: () => true,
              orElse: () => false,
            )) ...[
              const Divider(height: 40),
              const Text('基础控制 (指令 0x03)'),
              const SizedBox(height: 16),
              Row(
                mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                children: [
                  // --- 开灯按钮 ---
                  ElevatedButton(
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.green[100],
                    ),
                    onPressed: _turnOn,
                    child: const Text('全开 (ON)'),
                  ),
                  // --- 关灯按钮 ---
                  ElevatedButton(
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.red[100],
                    ),
                    onPressed: _turnOff,
                    child: const Text('全关 (OFF)'),
                  ),
                ],
              ),
            ],
          ],
        ),
      ),
    );
  }

  // --- 4. 事件处理方法 ---

  void _connect() {
    // 调用 Controller 的方法来改变状态
    ref.read(connectionProvider.notifier).connect(_ipController.text, 8080);
  }

  void _disconnect() {
    ref.read(connectionProvider.notifier).disconnect();
  }

  void _turnOn() {
    // 直接调用 Service 的方法“即发即忘”
    ref.read(ledMatrixServiceProvider).sendToggleCommand(true);
  }

  void _turnOff() {
    ref.read(ledMatrixServiceProvider).sendToggleCommand(false);
  }
}

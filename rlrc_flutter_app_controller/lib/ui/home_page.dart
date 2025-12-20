import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../controllers/connection_controller.dart';
import '../service/led_matrix_service.dart';
import '../service/device_discovery_service.dart'; // [新增] 引入发现服务

class HomePage extends ConsumerStatefulWidget {
  const HomePage({super.key});

  @override
  ConsumerState<HomePage> createState() => _HomePageState();
}

class _HomePageState extends ConsumerState<HomePage> {
  late final TextEditingController _ipController;

  // [新增] 扫描状态
  bool _isScanning = false;

  // [新增] 画板状态
  // 5x5 = 25 颗灯，初始化全黑
  List<Color> _pixels = List.filled(25, Colors.black);
  Color _selectedColor = Colors.red; // 默认画笔颜色
  DateTime _lastSendTime = DateTime.now(); // 用于节流

  @override
  void initState() {
    super.initState();
    // 默认预填一个 IP，方便调试
    _ipController = TextEditingController(text: '192.168.57.179');
  }

  @override
  void dispose() {
    _ipController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final connectionState = ref.watch(connectionProvider);
    final isConnected = connectionState.maybeWhen(
      connected: () => true,
      orElse: () => false,
    );

    return Scaffold(
      appBar: AppBar(
        title: const Text('STM32 画板 & 控制台'),
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
        actions: [
           // [新增] 连接状态指示灯
           Padding(
             padding: const EdgeInsets.only(right: 16),
             child: Icon(
               isConnected ? Icons.wifi : Icons.wifi_off,
               color: isConnected ? Colors.green : Colors.grey,
             ),
           )
        ],
      ),
      body: SingleChildScrollView( // [修改] 防止键盘或画板溢出
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            // ================== 1. 连接区域 (带扫描) ==================
            Row(
              children: [
                Expanded(
                  child: TextField(
                    controller: _ipController,
                    decoration: const InputDecoration(
                      labelText: '设备 IP 地址',
                      hintText: '例如 192.168.1.100',
                      border: OutlineInputBorder(),
                      contentPadding: EdgeInsets.symmetric(horizontal: 12, vertical: 0),
                    ),
                    enabled: !isConnected,
                  ),
                ),
                const SizedBox(width: 8),
                // [新增] 扫描按钮
                if (!isConnected)
                  FilledButton.tonal(
                    onPressed: _isScanning ? null : _scanForDevice,
                    child: _isScanning
                      ? const SizedBox(width: 20, height: 20, child: CircularProgressIndicator(strokeWidth: 2))
                      : const Icon(Icons.search),
                  ),
              ],
            ),
            const SizedBox(height: 12),

            // 连接/断开 按钮
            connectionState.when(
              initial: () => FilledButton(
                onPressed: _connect,
                child: const Text('连接设备 (端口 8080)'),
              ),
              connecting: () => const Center(child: CircularProgressIndicator()),
              connected: () => FilledButton(
                style: FilledButton.styleFrom(backgroundColor: Colors.red.shade100, foregroundColor: Colors.red),
                onPressed: _disconnect,
                child: const Text('断开连接'),
              ),
              error: (msg) => Column(
                children: [
                  Text('错误: $msg', style: const TextStyle(color: Colors.red)),
                  TextButton(onPressed: _connect, child: const Text('重试')),
                ],
              ),
            ),

            const Divider(height: 32),

            // ================== 2. 功能区域 (仅连接后显示) ==================
            if (isConnected) ...[
              const Text('🎨 像素画板 (5x5)', style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
              const SizedBox(height: 12),

              // 2.1 颜色选择器
              SingleChildScrollView(
                scrollDirection: Axis.horizontal,
                child: Row(
                  children: [
                    _buildColorBtn(Colors.red),
                    _buildColorBtn(Colors.green),
                    _buildColorBtn(Colors.blue),
                    _buildColorBtn(Colors.yellow),
                    _buildColorBtn(Colors.purple),
                    _buildColorBtn(Colors.white),
                    _buildColorBtn(Colors.black, label: '擦除'), // 黑色即关灯
                  ],
                ),
              ),
              const SizedBox(height: 16),

              // 2.2 画板网格 (核心交互)
              Center(
                child: GestureDetector(
                  // [核心] 监听手指在画板上的滑动
                  onPanUpdate: (details) => _handlePan(details, context),
                  onPanDown: (details) => _handlePan(details, context), // 支持点按
                  child: SizedBox(
                    width: 300,
                    height: 300,
                    child: GridView.builder(
                      physics: const NeverScrollableScrollPhysics(), // 禁止 Grid 滚动
                      gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
                        crossAxisCount: 5,
                        mainAxisSpacing: 4,
                        crossAxisSpacing: 4,
                      ),
                      itemCount: 25,
                      itemBuilder: (context, index) {
                        return Container(
                          decoration: BoxDecoration(
                            color: _pixels[index],
                            borderRadius: BorderRadius.circular(4),
                            border: Border.all(color: Colors.grey.shade300),
                            boxShadow: [
                              if (_pixels[index] != Colors.black)
                                BoxShadow(color: _pixels[index].withOpacity(0.5), blurRadius: 8)
                            ]
                          ),
                        );
                      },
                    ),
                  ),
                ),
              ),

              const SizedBox(height: 16),
              // 全清按钮
              OutlinedButton.icon(
                onPressed: _clearBoard,
                icon: const Icon(Icons.delete_outline),
                label: const Text('清空画板'),
              ),
            ],
          ],
        ),
      ),
    );
  }

  // --- UI 组件构建助手 ---
  Widget _buildColorBtn(Color color, {String? label}) {
    final isSelected = _selectedColor == color;
    return Padding(
      padding: const EdgeInsets.only(right: 8),
      child: InkWell(
        onTap: () => setState(() => _selectedColor = color),
        child: Container(
          width: 40, height: 40,
          decoration: BoxDecoration(
            color: color,
            shape: BoxShape.circle,
            border: isSelected ? Border.all(color: Colors.blue, width: 3) : Border.all(color: Colors.grey.shade300),
          ),
          child: label != null && isSelected
              ? const Icon(Icons.check, color: Colors.grey, size: 20)
              : null,
        ),
      ),
    );
  }

  // --- 逻辑处理 ---

  // 1. 扫描设备
  Future<void> _scanForDevice() async {
    setState(() => _isScanning = true);
    try {
      // 监听 Stream，取第一个结果
      // 注意：这里需要配合上一轮我们写的 DeviceDiscoveryService
      final ip = await DeviceDiscoveryService.scanDevices().first.timeout(const Duration(seconds: 3));

      if (mounted) {
        _ipController.text = ip;
        ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text('发现设备: $ip')));
        // 可选：自动连接
        // _connect();
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(const SnackBar(content: Text('未发现设备，请检查是否在同一 WiFi')));
      }
    } finally {
      if (mounted) setState(() => _isScanning = false);
    }
  }

  // 2. 连接
  void _connect() {
    ref.read(connectionProvider.notifier).connect(_ipController.text, 8080);
  }

  void _disconnect() {
    ref.read(connectionProvider.notifier).disconnect();
  }

  // 3. 触摸处理 (画板算法核心)
  void _handlePan(dynamic details, BuildContext context) {
    // 这里的 300 是 SizedBox 的宽度，如果上面改了这里也要改
    // 更好的做法是用 LayoutBuilder 获取实际尺寸，这里为了大作业演示简单处理
    const boardSize = 300.0;
    const gridSize = 5;

    // 获取触摸点相对于 Grid 的坐标
    final RenderBox box = context.findRenderObject() as RenderBox;
    // 注意：需要找到 GridView 的 RenderBox，这里简化处理，假设 SizedBox 是 body 的一部分
    // 实际上 GestureDetector 包裹了 SizedBox，localPosition 就是相对于 300x300 的
    final localPos = details.localPosition;

    if (localPos.dx < 0 || localPos.dx >= boardSize || localPos.dy < 0 || localPos.dy >= boardSize) {
      return; // 超出范围
    }

    final cellSize = boardSize / gridSize;
    final x = (localPos.dx / cellSize).floor();
    final y = (localPos.dy / cellSize).floor();
    final index = y * gridSize + x;

    if (index >= 0 && index < 25) {
      // 如果颜色变了，更新 UI
      if (_pixels[index] != _selectedColor) {
        setState(() {
          _pixels[index] = _selectedColor;
        });
        // 触发发送 (带节流)
        _throttledSend();
      }
    }
  }

  void _clearBoard() {
    setState(() {
      _pixels = List.filled(25, Colors.black);
    });
    _throttledSend(force: true); // 强制立即发送
  }

  // 4. 发送逻辑 (节流阀)
  void _throttledSend({bool force = false}) {
    final now = DateTime.now();
    // 限制每 50ms 发送一次 (约 20FPS)，防止阻塞 UART
    if (force || now.difference(_lastSendTime).inMilliseconds > 50) {
      _lastSendTime = now;

      // 准备数据：将 List<Color> 转换为 List<int> (R,G,B, R,G,B...)
      final List<int> frameData = [];
      for (var color in _pixels) {
        frameData.add(color.red);
        frameData.add(color.green);
        frameData.add(color.blue);
      }

      // 调用 Service
      ref.read(ledMatrixServiceProvider).sendFullFrame(frameData);
    }
  }
}
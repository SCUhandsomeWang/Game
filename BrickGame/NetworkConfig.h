#ifndef NETWORKCONFIG_H
#define NETWORKCONFIG_H

#include <cstdint>
#include <string>

// 网络配置常量
namespace NetworkConfig {
    // 默认网络设置
    constexpr uint16_t DEFAULT_PORT = 5555;
    constexpr const char* DEFAULT_HOST = "127.0.0.1";
    constexpr const char* LOCALHOST = "127.0.0.1";
    
    // 网络性能设置
    constexpr float STATE_UPDATE_RATE = 30.0f;  // Hz - 每秒发送状态更新30次
    constexpr float PADDLE_UPDATE_RATE = 60.0f; // Hz - 板位置更新频率
    constexpr float CONNECTION_TIMEOUT = 5.0f;  // 秒 - 连接超时
    
    // 插值平滑参数
    constexpr float BALL_INTERPOLATION_SPEED = 0.2f;    // 球的平滑速度
    constexpr float PADDLE_INTERPOLATION_SPEED = 0.25f; // 板的平滑速度
    
    // 网络缓冲区大小
    constexpr size_t MAX_MESSAGE_SIZE = 1024;
    constexpr size_t MAX_PLAYERS = 2;  // 当前支持最多2个玩家
    
    // 游戏网络规则
    constexpr bool AUTO_RESYNC_ON_DESYNC = true;  // 自动重新同步
    constexpr float MAX_POSITION_DEVIATION = 50.0f;  // 最大位置偏差（像素）
    constexpr float MAX_VELOCITY_DEVIATION = 50.0f;  // 最大速度偏差
    
    // 调试信息
    constexpr bool ENABLE_NETWORK_DEBUG = true;  // 启用网络调试输出
    constexpr bool SHOW_INTERPOLATION_INFO = false;  // 显示插值信息
}

#endif // NETWORKCONFIG_H

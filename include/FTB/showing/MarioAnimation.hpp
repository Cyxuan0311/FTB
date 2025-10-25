#ifndef MARIO_ANIMATION_HPP
#define MARIO_ANIMATION_HPP

#include <ftxui/dom/elements.hpp>
#include <vector>
#include <string>

namespace FTB {

/**
 * @class MarioAnimation
 * @brief 马里奥跳跃动画类，提供完整的12帧跳跃动画效果
 * 
 * 功能特性：
 * - 12帧完整跳跃动画（蓄力→起跳→上升→漂浮→下落→着地→回稳）
 * - 动态背景效果（云朵移动、草地震动、山丘装饰）
 * - 优雅的灰色主题配色
 * - 流畅的动画播放控制
 */
class MarioAnimation {
public:
    /**
     * @brief 构造函数，初始化动画数据
     */
    MarioAnimation();

    /**
     * @brief 获取当前动画帧的渲染元素
     * @param wave_progress 动画进度（0.0 - 2π）
     * @return 渲染元素
     */
    ftxui::Element render(double wave_progress);

    /**
     * @brief 获取动画帧数
     * @return 总帧数
     */
    size_t getFrameCount() const;

    /**
     * @brief 设置动画速度
     * @param speed 动画速度（建议值：0.05-0.15）
     */
    void setAnimationSpeed(double speed);

    /**
     * @brief 获取动画速度
     * @return 当前动画速度
     */
    double getAnimationSpeed() const;

    /**
     * @brief 启用高帧率模式
     * @param enable 是否启用高帧率模式
     */
    void setHighFrameRateMode(bool enable);

    /**
     * @brief 检查是否启用高帧率模式
     * @return 是否启用高帧率模式
     */
    bool isHighFrameRateMode() const;

private:
    std::vector<std::vector<std::string>> frames_;  // 动画帧数据
    int current_frame_;                             // 当前帧索引
    double animation_speed_;                        // 动画速度
    bool high_frame_rate_mode_;                    // 高帧率模式标志

    /**
     * @brief 初始化12帧完整跳跃动画数据
     */
    void initializeFrames();

    /**
     * @brief 应用颜色主题到文本行
     * @param line 文本行
     * @return 带颜色的元素
     */
    ftxui::Element applyColorTheme(const std::string& line);
};

} // namespace FTB

#endif // MARIO_ANIMATION_HPP


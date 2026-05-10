#include "Item.h"
#include <cmath>
#include <cstdlib>

using namespace std;
using namespace sf;

Item::Item(float startX, float startY) {
    x = startX; y = startY;

    // 【强化爆出效果】更大的随机横向扩散，更高的初始反弹高度！
    vx = (rand() % 60 - 30) / 10.0f;
    vy = -4.0f - (rand() % 30) / 10.0f;
    active = true;

    sprite.setTexture(texItemPower);
    sprite.setOrigin(texItemPower.getSize().x / 2.0f, texItemPower.getSize().y / 2.0f);
    sprite.setScale(1.5f, 1.5f); // 稍微放大一点
}

void Item::update(float player_x, float player_y) {
    // 1. 重力下坠效果
    vy += 0.15f;
    if (vy > 4.5f) vy = 4.5f; // 限制最大下坠速度

    // 2. 【核心机制1】：越线全屏自动收集！
    // 只要玩家飞到屏幕上方 Y < 200 的位置，所有道具以极快速度飞向玩家
    if (player_y < 200.0f) {
        float dx = player_x - x;
        float dy = player_y - y;
        float dist = sqrt(dx * dx + dy * dy);
        if (dist > 0) {
            vx = (dx / dist) * 16.0f; // 吸附速度拉满！
            vy = (dy / dist) * 16.0f;
        }
    }

    x += vx; y += vy;
    if (y > PLAY_HEIGHT + 50) active = false; // 掉出屏幕底端后销毁
}

void Item::draw(RenderWindow& window) {
    sprite.setPosition(x + PLAY_OFFSET_X, y + PLAY_OFFSET_Y);
    window.draw(sprite);
}
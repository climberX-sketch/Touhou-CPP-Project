#include "Player.h"

using namespace std;
using namespace sf;

Player::Player() {
    x = PLAY_WIDTH / 2.0f; y = PLAY_HEIGHT - 80.0f; hit_radius = 5.0f;
    lives = 99999; bombs = 2; invincible_timer = 0; shoot_timer = 0; focus_mode = false;
    score = 0;
    graze = 0;
    power = 0; // 【初始化火力为 0】

    sprite.setTexture(texReimu);
    sprite.setOrigin(texReimu.getSize().x / 2.0f, texReimu.getSize().y / 2.0f);
    sprite.setScale(64.0f / texReimu.getSize().x, 64.0f / texReimu.getSize().y);
}

void Player::update(vector<Bullet*>& p_bullets) {
    focus_mode = Keyboard::isKeyPressed(Keyboard::LShift) || Keyboard::isKeyPressed(Keyboard::RShift) || Keyboard::isKeyPressed(Keyboard::LControl) || Keyboard::isKeyPressed(Keyboard::C);

    float speed = focus_mode ? 5.0f : 11.0f;
    if (Keyboard::isKeyPressed(Keyboard::Up) && y > 25) y -= speed;
    if (Keyboard::isKeyPressed(Keyboard::Down) && y < PLAY_HEIGHT - 25) y += speed;
    if (Keyboard::isKeyPressed(Keyboard::Left) && x > 25) x -= speed;
    if (Keyboard::isKeyPressed(Keyboard::Right) && x < PLAY_WIDTH - 25) x += speed;

    if (shoot_timer > 0) shoot_timer--;
    if (Keyboard::isKeyPressed(Keyboard::Z) && shoot_timer <= 0) { // 注意这里改成 <= 0 更严谨
        sndShoot.play();

        int power_level = power / 10;

        if (focus_mode) {
            // ==========================================
            // 【直线弹模式】(按住Shift)
            // 特点：射速极快，伤害极高，必须贴脸瞄准
            // ==========================================
            p_bullets.push_back(new PlayerBullet(x - 12, y));
            p_bullets.push_back(new PlayerBullet(x + 12, y));
            if (power_level >= 1) {
                p_bullets.push_back(new PlayerBullet(x - 24, y + 10));
                p_bullets.push_back(new PlayerBullet(x + 24, y + 10));
            }
            if (power_level >= 2) {
                p_bullets.push_back(new PlayerBullet(x - 36, y + 20));
                p_bullets.push_back(new PlayerBullet(x + 36, y + 20));
            }

            shoot_timer = 8; // 【独立射速】冷却极短，像激光一样倾泻！
        }
        else {
            // ==========================================
            // 【追踪弹模式】(不按Shift)
            // 特点：射速慢(冷却长)，呈优美扇形，必定全中
            // ==========================================
            // 修改了初始的 vx 和 vy，让扇形展开得更好看，且不会飞得太偏
            p_bullets.push_back(new PlayerHomingBullet(x, y, -2.0f, -12.0f));
            p_bullets.push_back(new PlayerHomingBullet(x, y, 2.0f, -12.0f));
            if (power_level >= 1) {
                p_bullets.push_back(new PlayerHomingBullet(x, y, -5.0f, -10.0f));
                p_bullets.push_back(new PlayerHomingBullet(x, y, 5.0f, -10.0f));
            }
            if (power_level >= 2) {
                p_bullets.push_back(new PlayerHomingBullet(x, y, -8.0f, -8.0f));
                p_bullets.push_back(new PlayerHomingBullet(x, y, 8.0f, -8.0f));
            }

            shoot_timer = 15; // 【独立射速】0.25秒才发射一次，极大地减少视觉污染！
        }
    }
    if (invincible_timer > 0) invincible_timer--;
}

void Player::draw(RenderWindow& window) {
    if (invincible_timer > 0 && (invincible_timer / 5) % 2 == 0) return;
    sprite.setPosition(x + PLAY_OFFSET_X, y + PLAY_OFFSET_Y);
    window.draw(sprite);
    if (focus_mode) {
        CircleShape core(hit_radius); core.setOrigin(hit_radius, hit_radius);
        core.setFillColor(Color(255, 255, 255)); core.setOutlineColor(Color::Black); core.setOutlineThickness(1);
        core.setPosition(x + PLAY_OFFSET_X, y + PLAY_OFFSET_Y); window.draw(core);
    }
}
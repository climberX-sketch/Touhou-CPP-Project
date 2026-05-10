#pragma once
#include "Globals.h"
#include <vector>
#include <cmath>

class Bullet {
public:
    float x, y, vx, vy;
    float speed;
    sf::Sprite sprite;
    bool active = true;
    bool is_grazed = false;
    int damage = 1;

    virtual ~Bullet() {}
    virtual void update(float target_x = 0, float target_y = 0, std::vector<Bullet*>* enemy_bullets = nullptr) = 0;
    virtual void draw(sf::RenderWindow& window) {
        sprite.setPosition(x + PLAY_OFFSET_X, y + PLAY_OFFSET_Y);
        window.draw(sprite);
    }
};

class PlayerBullet : public Bullet {
public:
    PlayerBullet(float startX, float startY) {
        x = startX; y = startY; speed = 25.0f;
        damage = 1;
        sprite.setTexture(texPlayerStraight);
        sprite.setOrigin(texPlayerStraight.getSize().x / 2.0f, texPlayerStraight.getSize().y / 2.0f);
        sprite.setScale(24.0f / texPlayerStraight.getSize().x, 24.0f / texPlayerStraight.getSize().y);
    }
    void update(float target_x = 0, float target_y = 0, std::vector<Bullet*>* enemy_bullets = nullptr) override {
        y -= speed;
        if (y < -50) active = false;
        sprite.rotate(15.0f);
    }
};

class PlayerHomingBullet : public Bullet {
public:
    float turn_speed;
    PlayerHomingBullet(float startX, float startY, float start_vx, float start_vy) {
        x = startX; y = startY;
        vx = start_vx * 1.5f; vy = start_vy * 1.5f;

        // 【核心修改】
        speed = 22.0f;       // 稍微加快飞行速度，让打击感更干脆 (原为18.0f)
        turn_speed = 3.5f;   // 史诗级强化转向能力！像毒蛇一样死死咬住目标 (原为1.0f)

        damage = 1; // 保持刮痧伤害，体现定位

        sprite.setTexture(texPlayerHoming);
        sprite.setOrigin(texPlayerHoming.getSize().x / 2.0f, texPlayerHoming.getSize().y / 2.0f);
        sprite.setScale(36.0f / texPlayerHoming.getSize().x, 24.0f / texPlayerHoming.getSize().y);
    }
    void update(float target_x = 0, float target_y = 0, std::vector<Bullet*>* enemy_bullets = nullptr) override {
        if (target_x != 0 && target_y != 0) {
            float dx = target_x - x; float dy = target_y - y;
            float dist = sqrt(dx * dx + dy * dy);
            if (dist > 0) { vx += (dx / dist) * turn_speed; vy += (dy / dist) * turn_speed; }
        }
        float mag = sqrt(vx * vx + vy * vy);
        if (mag > 0) { vx = (vx / mag) * speed; vy = (vy / mag) * speed; }
        x += vx; y += vy;
        sprite.setRotation(atan2(vy, vx) * 180.0f / 3.14159f);
        if (y < -50 || x < -50 || x > PLAY_WIDTH + 50 || y > PLAY_HEIGHT + 50) active = false;
    }
};

class EnemyBullet : public Bullet {
public:
    bool is_star;
    EnemyBullet(float startX, float startY, float angle, float spd, sf::Texture* tex, bool star = false, sf::Color tint = sf::Color(255, 255, 255)) {
        x = startX; y = startY; speed = spd * 1.5f;
        vx = cos(angle) * speed; vy = sin(angle) * speed;
        is_star = star;
        sprite.setTexture(*tex);
        sprite.setOrigin(tex->getSize().x / 2.0f, tex->getSize().y / 2.0f);
        sprite.setScale(36.0f / tex->getSize().x, 36.0f / tex->getSize().y);
        sprite.setColor(tint);
        if (!is_star) sprite.setRotation(angle * 180.0f / 3.14159f + 90.0f);
    }
    void update(float target_x = 0, float target_y = 0, std::vector<Bullet*>* enemy_bullets = nullptr) override {
        x += vx; y += vy;
        if (is_star) sprite.rotate(5.0f);
        if (y > PLAY_HEIGHT + 50 || x < -50 || x > PLAY_WIDTH + 50) active = false;
    }
};

class EnemyGravityBullet : public EnemyBullet {
public:
    int timer = 0;
    EnemyGravityBullet(float startX, float startY, float angle, float spd, sf::Texture* tex, bool star = false, sf::Color tint = sf::Color(255, 255, 255))
        : EnemyBullet(startX, startY, angle, spd, tex, star, tint) {
    }
    void update(float target_x = 0, float target_y = 0, std::vector<Bullet*>* enemy_bullets = nullptr) override {
        timer++;
        if (timer < 30) { vx *= 0.92f; vy *= 0.92f; }
        else { vy += 0.14f; if (vy > 7.0f) vy = 7.0f; }
        x += vx; y += vy;
        if (is_star) sprite.rotate(5.0f);
        else sprite.setRotation(atan2(vy, vx) * 180.0f / 3.14159f + 90.0f);
        if (y > PLAY_HEIGHT + 50 || x < -50 || x > PLAY_WIDTH + 50) active = false;
    }
};

class EnemyExplosiveBullet : public EnemyBullet {
public:
    int timer = 50; bool exploded = false;
    EnemyExplosiveBullet(float startX, float startY, float angle, float spd, sf::Texture* tex, bool star = false, sf::Color tint = sf::Color(255, 255, 255))
        : EnemyBullet(startX, startY, angle, spd, tex, star, tint) {
        sprite.setScale(72.0f / tex->getSize().x, 72.0f / tex->getSize().y);
    }
    void update(float target_x = 0, float target_y = 0, std::vector<Bullet*>* enemy_bullets = nullptr) override {
        timer--;
        if (timer <= 0 && !exploded) {
            exploded = true;
            if (enemy_bullets) {
                for (int i = 0; i < 12; i++) {
                    enemy_bullets->push_back(new EnemyBullet(x, y, i * (3.14159f * 2 / 12), 3.0f, &texStarPurple, true, sf::Color(210, 130, 255)));
                }
            }
            active = false;
        }
        x += vx; y += vy;
        if (is_star) sprite.rotate(5.0f);
    }
};

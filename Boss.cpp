#include "Boss.h"
#include <cstdlib>

using namespace std;
using namespace sf;

Boss::Boss() {
    x = PLAY_WIDTH / 2.0f; y = 120.0f; radius = 40.0f;
    max_hp = 3000; hp = max_hp; phase = 1;
    timer = 0; laser_timer = 0; laser_active = false;
    spell_timer = 0; spell_name = "";
    sprite.setTexture(texMarisa);
    sprite.setOrigin(texMarisa.getSize().x / 2.0f, texMarisa.getSize().y / 2.0f);
    sprite.setScale(80.0f / texMarisa.getSize().x, 80.0f / texMarisa.getSize().y);
}

void Boss::update(float px, float py, vector<Bullet*>& enemy_bullets) {
    timer++;
    int next_phase = phase;
    if (hp > 2250) next_phase = 1;
    else if (hp > 1500) next_phase = 2;
    else if (hp > 750) next_phase = 3;
    else next_phase = 4;

    if (next_phase == 4 && phase != 4) {
        spell_timer = 150;
        spell_name = "Love Sign \"Master Spark\"";
    }
    phase = next_phase;
    if (spell_timer > 0) spell_timer--;

    // ==========================================
    // 【优化2】全新智能走位 AI
    // ==========================================
    if (phase < 4) {
        // 1. 生成随机游荡坐标 (利用三角函数产生上下左右的不可控轨迹)
        float wander_x = (PLAY_WIDTH / 2.0f) + sin(timer / 45.0f) * 220.0f;
        float wander_y = 120.0f + cos(timer / 35.0f) * 80.0f;

        // 2. 混合追踪逻辑：60%随机乱飞 + 40%向玩家逼近
        float target_x = wander_x * 0.6f + px * 0.4f;
        float target_y = wander_y * 0.8f + py * 0.2f; // Y轴逼近的比例小一点，防止靠太近

        // 3. 限制活动死角 (防贴脸杀和出界)
        if (target_x < 50.0f) target_x = 50.0f;
        if (target_x > PLAY_WIDTH - 50.0f) target_x = PLAY_WIDTH - 50.0f;
        if (target_y < 50.0f) target_y = 50.0f;
        if (target_y > PLAY_HEIGHT * 0.4f) target_y = PLAY_HEIGHT * 0.4f; // Boss绝对不允许越过屏幕上半区(40%处)

        // 4. 平滑地飞向目标点
        x += (target_x - x) * 0.035f;
        y += (target_y - y) * 0.035f;
    }
    else {
        // 【关键】阶段4（魔炮阶段）必须强行回归中场，并稍微下压一点，营造大招压迫感
        x += ((PLAY_WIDTH / 2.0f) - x) * 0.05f;
        y += (150.0f - y) * 0.05f;
    }
    // ==========================================

    if (phase == 1) {
        if (timer % 40 == 0) for (int i = 0; i < 10; i++) enemy_bullets.push_back(new EnemyBullet(x, y, i * (3.14159f * 2 / 10) + (timer * 0.1f), 3.0f, &texBflyBlue, false, Color(150, 180, 255)));
        if (timer % 60 == 0) enemy_bullets.push_back(new EnemyBullet(x, y, atan2(py - y, px - x), 6.0f, &texBflyPurple, false, Color(210, 130, 255)));
    }
    else if (phase == 2) {
        if (timer % 4 == 0) {
            Texture* current_tex = (rand() % 2 == 0) ? &texStarBlue : &texStarPurple;
            Color current_tint = (current_tex == &texStarBlue) ? Color(150, 180, 255) : Color(210, 130, 255);
            enemy_bullets.push_back(new EnemyGravityBullet(x, y, (rand() % 360) * 3.14159f / 180.0f, 5.0f + (rand() % 5) / 1.0f, current_tex, true, current_tint));
        }
    }
    else if (phase == 3) {
        if (timer % 60 == 0) {
            float ang = atan2(py - y, px - x);
            enemy_bullets.push_back(new EnemyExplosiveBullet(x, y, ang - 0.2f, 5.0f, &texBflyRed, false, Color(255, 130, 130)));
            enemy_bullets.push_back(new EnemyExplosiveBullet(x, y, ang + 0.2f, 5.0f, &texBflyRed, false, Color(255, 130, 130)));
        }
    }
    else if (phase == 4) {
        if (spell_timer <= 0) {
            laser_timer++;
            if (laser_timer < 120) {
                laser_active = false;
                if (laser_timer % 15 == 0) for (int i = 0; i < 32; i++) enemy_bullets.push_back(new EnemyBullet(x, y, i * (3.14159f * 2 / 32) + (laser_timer * 0.05f), 1.5f, &texStarPurple, true, Color(210, 130, 255)));
            }
            else if (laser_timer < 420) {
                laser_active = true;
                laser_rect = FloatRect(x - 75, y, 150, PLAY_HEIGHT);
                if (laser_timer % 4 == 0) {
                    for (int i = 0; i < 5; i++) {
                        enemy_bullets.push_back(new EnemyBullet(x, y, i * (3.14159f * 2 / 5) + (laser_timer * 0.06f), 5.5f, &texStarBlue, true, Color(150, 180, 255)));
                        enemy_bullets.push_back(new EnemyBullet(x, y, i * (3.14159f * 2 / 5) - (laser_timer * 0.06f), 4.0f, &texStarPurple, true, Color(210, 130, 255)));
                    }
                }
                if (laser_timer % 20 == 0) {
                    enemy_bullets.push_back(new EnemyGravityBullet(x, y, (rand() % 90) * 3.14159f / 180.0f, 3.0f + (rand() % 4), &texStarBlue, true, Color(150, 180, 255)));
                    enemy_bullets.push_back(new EnemyGravityBullet(x, y, (90 + rand() % 90) * 3.14159f / 180.0f, 3.0f + (rand() % 4), &texStarPurple, true, Color(210, 130, 255)));
                }
            }
            else if (laser_timer < 500) {
                laser_active = false;
                if (laser_timer == 421) {
                    for (int i = 0; i < 8; i++) enemy_bullets.push_back(new EnemyExplosiveBullet(x, y, i * (3.14159f * 2 / 8), 3.5f, &texBflyRed, false, Color(255, 130, 130)));
                }
            }
            else { laser_timer = 0; }
        }
    }
}

void Boss::draw(RenderWindow& window) {
    if (phase == 4) {
        if (laser_timer > 0 && laser_timer < 120) {
            CircleShape warn(std::max(1.0f, 180.0f - laser_timer * 1.5f)); warn.setOrigin(warn.getRadius(), warn.getRadius()); warn.setPosition(x + PLAY_OFFSET_X, y + PLAY_OFFSET_Y); warn.setFillColor(Color::Transparent); warn.setOutlineColor(Color::Yellow); warn.setOutlineThickness(4); window.draw(warn);
            RectangleShape line(Vector2f(max(1.0f, laser_timer / 8.0f), PLAY_HEIGHT)); line.setFillColor(Color(255, 50, 50)); line.setOrigin(line.getSize().x / 2.0f, 0); line.setPosition(x + PLAY_OFFSET_X, y + PLAY_OFFSET_Y); window.draw(line);
        }
        else if (laser_active) {
            float jitter = (rand() % 12) - 6.0f;
            float fade = (laser_timer > 400) ? (420 - laser_timer) / 20.0f : 1.0f;

            RectangleShape beamAura(Vector2f(210, PLAY_HEIGHT));
            int rb = (timer * 15) % 255;
            beamAura.setFillColor(Color(100 + rb / 2, 200, 255 - rb / 2, 100 * fade));
            beamAura.setOrigin(105, 0);
            beamAura.setPosition(x + jitter + PLAY_OFFSET_X, y + PLAY_OFFSET_Y + 10);
            window.draw(beamAura, BlendAdd);

            Sprite runesLeft(texLaserBorder);
            runesLeft.setOrigin(texLaserBorder.getSize().x / 2.0f, 0);
            runesLeft.setPosition(x - 90 + jitter + PLAY_OFFSET_X, y + PLAY_OFFSET_Y + 10);
            runesLeft.setScale(2.5f, PLAY_HEIGHT / texLaserBorder.getSize().y);
            runesLeft.setColor(Color(255, 255, 255, 220 * fade));
            window.draw(runesLeft, BlendAdd);

            Sprite runesRight(texLaserBorder);
            runesRight.setOrigin(texLaserBorder.getSize().x / 2.0f, 0);
            runesRight.setPosition(x + 90 + jitter + PLAY_OFFSET_X, y + PLAY_OFFSET_Y + 10);
            runesRight.setScale(2.5f, PLAY_HEIGHT / texLaserBorder.getSize().y);
            runesRight.setColor(Color(255, 255, 255, 220 * fade));
            window.draw(runesRight, BlendAdd);

            Sprite beamMain(texLaserBody);
            beamMain.setOrigin(texLaserBody.getSize().x / 2.0f, 0);
            beamMain.setPosition(x + jitter + PLAY_OFFSET_X, y + PLAY_OFFSET_Y + 10);
            beamMain.setScale(150.0f / texLaserBody.getSize().x, PLAY_HEIGHT / texLaserBody.getSize().y);
            beamMain.setColor(Color(255, 255, 255, 255 * fade));
            window.draw(beamMain, BlendAdd);

            RectangleShape beamCore(Vector2f(60, PLAY_HEIGHT));
            beamCore.setFillColor(Color(255, 255, 255, 255 * fade));
            beamCore.setOrigin(30, 0);
            beamCore.setPosition(x + jitter + PLAY_OFFSET_X, y + PLAY_OFFSET_Y + 10);
            window.draw(beamCore);
        }
    }

    sprite.setPosition(x + PLAY_OFFSET_X, y + PLAY_OFFSET_Y);
    window.draw(sprite);

    if (spell_timer > 0) {
        int alpha = spell_timer > 120 ? (150 - spell_timer) * 8 : (spell_timer < 30 ? spell_timer * 8 : 240);
        Sprite bgSpell(texSpellBg);
        bgSpell.setTextureRect(IntRect(spell_timer * 3, spell_timer * 2, PLAY_WIDTH, PLAY_HEIGHT));
        bgSpell.setPosition(PLAY_OFFSET_X, PLAY_OFFSET_Y);
        bgSpell.setColor(Color(255, 255, 255, std::min(255, std::max(0, alpha))));
        window.draw(bgSpell);

        Sprite cutIn(texCutin);
        cutIn.setOrigin(texCutin.getSize().x / 2.0f, texCutin.getSize().y / 2.0f);
        float scale = 0.65f;
        cutIn.setScale(scale, scale);
        cutIn.setColor(Color(255, 255, 255, std::min(255, std::max(0, alpha))));

        float slideX = PLAY_OFFSET_X + PLAY_WIDTH + 200 - (150 - spell_timer) * 8.0f;
        cutIn.setPosition(slideX, PLAY_OFFSET_Y + PLAY_HEIGHT / 2.0f);
        window.draw(cutIn);

        drawOutlinedText(window, spell_name, 45, Color(255, 255, 150, std::min(255, std::max(0, alpha))), PLAY_OFFSET_X + PLAY_WIDTH / 2.0f, PLAY_OFFSET_Y + PLAY_HEIGHT / 2.0f + 150, true);
    }

    RectangleShape bgBar(Vector2f(PLAY_WIDTH - 40, 6)); bgBar.setFillColor(Color(100, 100, 100)); bgBar.setPosition(20 + PLAY_OFFSET_X, 15 + PLAY_OFFSET_Y); window.draw(bgBar);
    if (hp > 0) {
        RectangleShape hpBar(Vector2f((float)hp / max_hp * (PLAY_WIDTH - 40), 6));
        hpBar.setFillColor(phase == 4 ? Color(255, 50, 50) : (phase == 3 ? Color(255, 200, 50) : Color(50, 255, 50)));
        hpBar.setPosition(20 + PLAY_OFFSET_X, 15 + PLAY_OFFSET_Y); window.draw(hpBar);
    }
}
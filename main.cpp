#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm> 
#include <string>

// 引入模块
#include "Globals.h"
#include "Bullets.h"
#include "Player.h"
#include "Boss.h"
#include "Item.h" // 引入我们新增的掉落物头文件

using namespace std;
using namespace sf;

int main() {
    RenderWindow window(VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "Touhou Clone - Power Up Edition");
    window.setFramerateLimit(60);

    // ================= 资源加载区 =================
    globalFont.loadFromFile("font.ttf");
    texReimu.loadFromFile("reimu.png");
    texMarisa.loadFromFile("marisa.png");
    texGlobalBg.loadFromFile("sakuya_bg.png");
    texGlobalBg.setRepeated(true);
    texPauseBg.loadFromFile("pause_bg.png");

    Image imgReimuSheet; imgReimuSheet.loadFromFile("reimu_sheet.png");
    texPlayerStraight.loadFromImage(imgReimuSheet, IntRect(82, 146, 14, 14));
    texPlayerHoming.loadFromImage(imgReimuSheet, IntRect(1, 148, 14, 9));

    texSpellBg.loadFromFile("Touhou_Eiyashou_Stage.png", IntRect(9, 3894, 512, 512));
    texSpellBg.setRepeated(true);
    Image imgCutinSheet;
    imgCutinSheet.loadFromFile("Touhou_Hyouibana_Portraits.png");
    imgCutinSheet.createMaskFromColor(Color::Black);
    texCutin.loadFromImage(imgCutinSheet, IntRect(2659, 1, 884, 668));

    Image imgProj;
    imgProj.loadFromFile("Projectiles_and_Items.png");
    imgProj.createMaskFromColor(Color::Black);

    texBflyPurple.loadFromImage(imgProj, IntRect(74, 180, 32, 30));
    texBflyBlue.loadFromImage(imgProj, IntRect(137, 179, 34, 30));
    texBflyRed.loadFromImage(imgProj, IntRect(41, 180, 33, 30));
    texStarBlue.loadFromImage(imgProj, IntRect(103, 271, 32, 34));
    texStarPurple.loadFromImage(imgProj, IntRect(71, 272, 33, 31));
    texLaserBody.loadFromImage(imgProj, IntRect(567, 8, 16, 129));
    texLaserBorder.loadFromImage(imgProj, IntRect(647, 7, 16, 131));

    // 【加载红色的P点】
    texItemPower.loadFromImage(imgProj, IntRect(311, 69, 17, 16));

    Image imgMenu; imgMenu.loadFromFile("menu_sheet.png"); imgMenu.createMaskFromColor(Color::Black); texMenu.loadFromImage(imgMenu, IntRect(2, 513, 507, 222));
    Image imgGO; imgGO.loadFromFile("gameover_sheet.png"); imgGO.createMaskFromColor(Color::White); texGameover.loadFromImage(imgGO, IntRect(0, 59, 188, 73));

    if (bufShoot.loadFromFile("001.wav")) sndShoot.setBuffer(bufShoot);
    if (bufHit.loadFromFile("002.wav")) sndHit.setBuffer(bufHit);
    if (bufPichun.loadFromFile("003.wav")) sndPichun.setBuffer(bufPichun);
    if (bufBomb.loadFromFile("002.wav")) sndBomb.setBuffer(bufBomb);
    bgm.openFromFile("bgm.mp3"); bgm.setLoop(true); bgm.play();

    // ================= 游戏变量区 =================
    Player player;
    Boss boss;
    vector<Bullet*> player_bullets;
    vector<Bullet*> enemy_bullets;

    string gameState = "menu";
    bool isPaused = false;
    int bomb_timer = 0;
    float bomb_x = 0, bomb_y = 0;
    float bg_scroll_y = 0.0f;

    // 【新增】道具系统变量
    vector<Item*> items;
    int last_phase = 1;

    // ================= 核心游戏循环 =================
    while (window.isOpen()) {
        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed) window.close();
            if (event.type == Event::KeyPressed) {
                if (gameState == "menu" && event.key.code == Keyboard::Enter) gameState = "playing";
                else if ((gameState == "game_over" || gameState == "victory") && event.key.code == Keyboard::Escape) window.close();
                else if ((gameState == "game_over" || gameState == "victory") && event.key.code == Keyboard::R) {
                    player = Player(); boss = Boss();
                    for (auto b : player_bullets) delete b; player_bullets.clear();
                    for (auto b : enemy_bullets) delete b; enemy_bullets.clear();
                    for (auto it : items) delete it; items.clear(); // 重置掉落物
                    gameState = "playing";
                    bomb_timer = 0;
                    bgm.play();
                }
                else if (gameState == "playing") {
                    if (event.key.code == Keyboard::Escape || event.key.code == Keyboard::P) {
                        isPaused = !isPaused; if (isPaused) bgm.pause(); else bgm.play();
                    }
                    if (event.key.code == Keyboard::X && !isPaused && player.bombs > 0 && bomb_timer <= 0) {
                        player.bombs--; bomb_timer = 60; bomb_x = player.x; bomb_y = player.y;
                        for (auto b : enemy_bullets) delete b; enemy_bullets.clear();
                        boss.hp -= 30; player.invincible_timer = 120; sndBomb.play(); sndHit.play();
                        if (boss.hp <= 0) { gameState = "victory"; bgm.stop(); }
                    }
                    if (event.key.code == Keyboard::N && !isPaused) {
                        boss.hp -= 500;
                        if (boss.hp <= 0) { gameState = "victory"; bgm.stop(); }
                    }
                }
            }
        }

        if (gameState == "playing" && !isPaused) {
            bg_scroll_y -= 2.0f;
            player.update(player_bullets);
            boss.update(player.x, player.y, enemy_bullets);

            if (bomb_timer > 0) { bomb_timer--; for (auto b : enemy_bullets) delete b; enemy_bullets.clear(); }

            // 1. 更新自机子弹与 Boss 碰撞
            // 【新增】用来累计对 Boss 造成的伤害
            static int damage_accumulator = 0;

            for (auto b : player_bullets) {
                b->update(boss.x, boss.y, nullptr);
                if (b->active && sqrt(pow(b->x - boss.x, 2) + pow(b->y - boss.y, 2)) < boss.radius) {

                    // 1. 扣除对应子弹的真实伤害！
                    boss.hp -= b->damage;

                    // 2. 累计造成的伤害
                    damage_accumulator += b->damage;

                    sndHit.play();
                    b->active = false;

                    // 3. 【修复】每累计造成 8 点伤害，就必定爆出一个 P 点！
                    while (damage_accumulator >= 8 && boss.phase < 4) {
                        items.push_back(new Item(boss.x, boss.y));
                        damage_accumulator -= 8; // 扣除消耗掉的积攒值
                    }

                    if (boss.hp <= 0) {
                        gameState = "victory"; bgm.stop();
                        break;
                    }
                }
            }

            // 2. Boss 换阶段，瞬间大爆金币 45 个！
            if (boss.phase != last_phase) {
                for (int i = 0; i < 45; i++) items.push_back(new Item(boss.x, boss.y));
                last_phase = boss.phase;
            }

            // 3. 更新道具坐标并处理玩家收集
            for (auto it = items.begin(); it != items.end(); ) {
                (*it)->update(player.x, player.y);

                float dist = sqrt(pow((*it)->x - player.x, 2) + pow((*it)->y - player.y, 2));
                if (dist < 50.0f) {
                    if (player.power < 30) player.power++; // 火力封顶 30
                    player.score += 50;
                    delete* it;
                    it = items.erase(it);
                }
                else if (!(*it)->active) {
                    delete* it;
                    it = items.erase(it);
                }
                else {
                    ++it;
                }
            }

            // 4. 更新敌机子弹与玩家碰撞、擦弹
            bool hit = false;
            vector<Bullet*> current_enemy_bullets = enemy_bullets;
            for (auto b : current_enemy_bullets) {
                b->update(player.x, player.y, &enemy_bullets);
                if (b->active) {
                    float dist = sqrt(pow(b->x - player.x, 2) + pow(b->y - player.y, 2));
                    if (dist < player.hit_radius + 6) {
                        hit = true;
                    }
                    else if (dist < player.hit_radius + 25 && !b->is_grazed) {
                        b->is_grazed = true;
                        player.graze++;
                        player.score += 100;
                    }
                }
            }

            // 魔炮光柱判定
            if (boss.phase == 4 && boss.laser_active && boss.laser_rect.contains(player.x, player.y)) hit = true;

            // 玩家死亡结算
            if (hit && player.invincible_timer <= 0) {
                player.lives--; sndPichun.play(); player.invincible_timer = 120;
                for (auto b : enemy_bullets) delete b; enemy_bullets.clear();

                // 死亡掉落 P 点惩罚 (经典机制)
                player.power = max(0, player.power - 5);

                if (player.lives <= 0) { gameState = "game_over"; bgm.stop(); }
            }

            // 清理失效实体
            player_bullets.erase(remove_if(player_bullets.begin(), player_bullets.end(), [](Bullet* b) { if (!b->active) { delete b; return true; } return false; }), player_bullets.end());
            enemy_bullets.erase(remove_if(enemy_bullets.begin(), enemy_bullets.end(), [](Bullet* b) { if (!b->active) { delete b; return true; } return false; }), enemy_bullets.end());
        }

        // ================= 渲染区 =================
        window.clear(Color::Black);

        Sprite bgGlobal(texGlobalBg);
        bgGlobal.setTextureRect(IntRect(0, (int)bg_scroll_y, texGlobalBg.getSize().x, texGlobalBg.getSize().y));
        bgGlobal.setScale(1280.0f / texGlobalBg.getSize().x, 720.0f / texGlobalBg.getSize().y);
        window.draw(bgGlobal);

        if (gameState == "menu") {
            Sprite sMenu(texMenu); sMenu.setOrigin(texMenu.getSize().x / 2.0f, texMenu.getSize().y / 2.0f);
            sMenu.setScale(608.0f / texMenu.getSize().x, 266.0f / texMenu.getSize().y);
            sMenu.setPosition(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f - 100); window.draw(sMenu);
            drawOutlinedText(window, "Press [ENTER] to Start", 50, Color::White, SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f + 150, true);
        }
        else {
            RectangleShape playArea(Vector2f(PLAY_WIDTH, PLAY_HEIGHT)); playArea.setPosition(PLAY_OFFSET_X, PLAY_OFFSET_Y);
            playArea.setFillColor(Color(0, 0, 0, 150));
            window.draw(playArea);

            boss.draw(window);
            player.draw(window);
            for (auto b : player_bullets) b->draw(window);
            for (auto b : enemy_bullets) b->draw(window);
            for (auto item : items) item->draw(window); // 绘制散落的P点！

            if (bomb_timer > 0) {
                CircleShape flash((60 - bomb_timer) * 18); flash.setOrigin(flash.getRadius(), flash.getRadius()); flash.setPosition(bomb_x + PLAY_OFFSET_X, bomb_y + PLAY_OFFSET_Y);
                flash.setFillColor(Color(255, 50, 50, 150)); window.draw(flash);
            }
            if (gameState == "game_over") {
                Sprite sGo(texGameover); sGo.setOrigin(texGameover.getSize().x / 2.0f, texGameover.getSize().y / 2.0f);
                sGo.setScale(470.0f / texGameover.getSize().x, 182.0f / texGameover.getSize().y);
                sGo.setPosition(PLAY_OFFSET_X + PLAY_WIDTH / 2.0f, PLAY_OFFSET_Y + PLAY_HEIGHT / 2.0f - 50); window.draw(sGo);
                drawOutlinedText(window, "Press [R] Retry or [ESC] Quit", 40, Color::White, PLAY_OFFSET_X + PLAY_WIDTH / 2.0f, PLAY_OFFSET_Y + PLAY_HEIGHT / 2.0f + 100, true);
            }
            else if (gameState == "victory") {
                RectangleShape overlay(Vector2f(PLAY_WIDTH, PLAY_HEIGHT)); overlay.setPosition(PLAY_OFFSET_X, PLAY_OFFSET_Y); overlay.setFillColor(Color(0, 50, 0, 180)); window.draw(overlay);
                drawOutlinedText(window, "YOU WIN!", 80, Color::Green, PLAY_OFFSET_X + PLAY_WIDTH / 2.0f, PLAY_OFFSET_Y + PLAY_HEIGHT / 2.0f - 20, true);
            }
            else if (isPaused) {
                Sprite sPause(texPauseBg); sPause.setPosition(PLAY_OFFSET_X, PLAY_OFFSET_Y);
                sPause.setScale((float)PLAY_WIDTH / texPauseBg.getSize().x, (float)PLAY_HEIGHT / texPauseBg.getSize().y); window.draw(sPause);
                RectangleShape overlay(Vector2f(PLAY_WIDTH, PLAY_HEIGHT)); overlay.setPosition(PLAY_OFFSET_X, PLAY_OFFSET_Y); overlay.setFillColor(Color(0, 0, 0, 120)); window.draw(overlay);
                drawOutlinedText(window, "- PAUSE -", 80, Color::White, PLAY_OFFSET_X + PLAY_WIDTH / 2.0f, PLAY_OFFSET_Y + PLAY_HEIGHT / 2.0f, true);
            }

            playArea.setFillColor(Color::Transparent); playArea.setOutlineColor(Color::White); playArea.setOutlineThickness(3); window.draw(playArea);

            // 右侧面板
            float uiX = PLAY_OFFSET_X + PLAY_WIDTH + 60;
            drawOutlinedText(window, "Score: " + to_string(player.score), 45, Color(200, 200, 255), uiX, 80);
            drawOutlinedText(window, "Graze: " + to_string(player.graze), 45, Color(200, 255, 200), uiX, 130);
            drawOutlinedText(window, "Power: " + to_string(player.power) + " / 30", 45, Color(255, 100, 100), uiX, 180);
            drawOutlinedText(window, "Lives: " + to_string(player.lives), 45, player.lives > 3 ? Color(255, 100, 100) : Color::White, uiX, 230);
            drawOutlinedText(window, "Bombs: " + to_string(player.bombs), 45, Color(100, 255, 100), uiX, 280);
            drawOutlinedText(window, "Boss HP: " + to_string(boss.hp), 45, Color(255, 255, 100), uiX, 380);
        }
        window.display();
    } 
    return 0;
}

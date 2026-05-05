#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm> // 修复：必须引入以使用 remove_if，防止其他电脑编译失败
#include <string>    // 修复：确保 to_string 正常工作

using namespace std;
using namespace sf;

// ================= 1. 标准化 16:9 高清排版常量 =================
const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;
const int PLAY_WIDTH = 600;
const int PLAY_HEIGHT = 660;
const int PLAY_OFFSET_X = 40;
const int PLAY_OFFSET_Y = 30;

// 全局资源
Texture texReimu, texMarisa, texStar, texPlayerStraight, texPlayerHoming, texGlobalBg, texPauseBg, texMenu, texGameover;
Font globalFont;
SoundBuffer bufShoot, bufHit, bufPichun, bufBomb;
Sound sndShoot, sndHit, sndPichun, sndBomb;
Music bgm;

void drawOutlinedText(RenderWindow& window, string str, int size, Color col, float x, float y, bool centered = false) {
    Text text(str, globalFont, size);
    text.setFillColor(col);
    text.setOutlineColor(Color::Black);
    text.setOutlineThickness(2.0f);
    if (centered) {
        FloatRect bounds = text.getLocalBounds();
        text.setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
    }
    text.setPosition(x, y);
    window.draw(text);
}

// ================= 2. 多态子弹系统 =================
class Bullet {
public:
    float x, y, vx, vy;
    float speed;
    Sprite sprite;
    bool active = true;

    virtual ~Bullet() {}
    virtual void update(float target_x = 0, float target_y = 0, vector<Bullet*>* enemy_bullets = nullptr) = 0;

    virtual void draw(RenderWindow& window) {
        sprite.setPosition(x + PLAY_OFFSET_X, y + PLAY_OFFSET_Y);
        window.draw(sprite);
    }
};

// 玩家：直线阴阳玉
class PlayerBullet : public Bullet {
public:
    PlayerBullet(float startX, float startY) {
        x = startX; y = startY; speed = 25.0f;
        sprite.setTexture(texPlayerStraight);
        sprite.setOrigin(texPlayerStraight.getSize().x / 2.0f, texPlayerStraight.getSize().y / 2.0f);
        sprite.setScale(24.0f / texPlayerStraight.getSize().x, 24.0f / texPlayerStraight.getSize().y);
    }
    void update(float target_x = 0, float target_y = 0, vector<Bullet*>* enemy_bullets = nullptr) override {
        y -= speed;
        if (y < -50) active = false;
        sprite.rotate(15.0f);
    }
};

// 玩家：自动追踪御札
class PlayerHomingBullet : public Bullet {
public:
    float turn_speed;
    PlayerHomingBullet(float startX, float startY, float start_vx, float start_vy) {
        x = startX; y = startY;
        vx = start_vx * 1.5f; vy = start_vy * 1.5f;
        speed = 18.0f; turn_speed = 1.0f;
        sprite.setTexture(texPlayerHoming);
        sprite.setOrigin(texPlayerHoming.getSize().x / 2.0f, texPlayerHoming.getSize().y / 2.0f);
        sprite.setScale(36.0f / texPlayerHoming.getSize().x, 24.0f / texPlayerHoming.getSize().y);
    }
    void update(float target_x = 0, float target_y = 0, vector<Bullet*>* enemy_bullets = nullptr) override {
        if (target_x != 0 && target_y != 0) {
            float dx = target_x - x;
            float dy = target_y - y;
            float dist = sqrt(dx * dx + dy * dy);
            if (dist > 0) {
                vx += (dx / dist) * turn_speed;
                vy += (dy / dist) * turn_speed;
            }
        }
        float mag = sqrt(vx * vx + vy * vy);
        if (mag > 0) {
            vx = (vx / mag) * speed;
            vy = (vy / mag) * speed;
        }
        x += vx; y += vy;
        sprite.setRotation(atan2(vy, vx) * 180.0f / 3.14159f);
        if (y < -50 || x < -50 || x > PLAY_WIDTH + 50 || y > PLAY_HEIGHT + 50) active = false;
    }
};

// 敌人：常规星星
class EnemyBullet : public Bullet {
public:
    EnemyBullet(float startX, float startY, float angle, float spd) {
        x = startX; y = startY; speed = spd * 1.5f;
        vx = cos(angle) * speed; vy = sin(angle) * speed;
        sprite.setTexture(texStar);
        sprite.setOrigin(texStar.getSize().x / 2.0f, texStar.getSize().y / 2.0f);
        sprite.setScale(36.0f / texStar.getSize().x, 36.0f / texStar.getSize().y);
    }
    void update(float target_x = 0, float target_y = 0, vector<Bullet*>* enemy_bullets = nullptr) override {
        x += vx; y += vy;
        if (y > PLAY_HEIGHT + 50 || x < -50 || x > PLAY_WIDTH + 50) active = false;
    }
};

// 敌人：重力抛物星
class EnemyGravityBullet : public EnemyBullet {
public:
    int timer = 0;
    EnemyGravityBullet(float startX, float startY, float angle, float spd) : EnemyBullet(startX, startY, angle, spd) {}
    void update(float target_x = 0, float target_y = 0, vector<Bullet*>* enemy_bullets = nullptr) override {
        timer++;
        if (timer < 30) {
            vx *= 0.92f; vy *= 0.92f;
        }
        else {
            vy += 0.14f;
            if (vy > 7.0f) vy = 7.0f;
        }
        x += vx; y += vy;
        if (y > PLAY_HEIGHT + 50 || x < -50 || x > PLAY_WIDTH + 50) active = false;
    }
};

// 敌人：定时炸裂星
class EnemyExplosiveBullet : public EnemyBullet {
public:
    int timer = 50;
    bool exploded = false;
    EnemyExplosiveBullet(float startX, float startY, float angle, float spd) : EnemyBullet(startX, startY, angle, spd) {
        sprite.setScale(72.0f / texStar.getSize().x, 72.0f / texStar.getSize().y);
    }
    void update(float target_x = 0, float target_y = 0, vector<Bullet*>* enemy_bullets = nullptr) override {
        timer--;
        if (timer <= 0 && !exploded) {
            exploded = true;
            if (enemy_bullets) {
                for (int i = 0; i < 12; i++) {
                    enemy_bullets->push_back(new EnemyBullet(x, y, i * (3.14159f * 2 / 12), 3.0f));
                }
            }
            active = false;
        }
        x += vx; y += vy;
    }
};

// ================= 3. 实体类 =================
class Boss {
public:
    float x, y, radius;
    int hp, max_hp, phase, timer, laser_timer;
    bool laser_active;
    FloatRect laser_rect;
    Sprite sprite;

    Boss() {
        x = PLAY_WIDTH / 2.0f; y = 120.0f; radius = 40.0f;
        max_hp = 2000; hp = max_hp; phase = 1;
        timer = 0; laser_timer = 0; laser_active = false;
        sprite.setTexture(texMarisa);
        sprite.setOrigin(texMarisa.getSize().x / 2.0f, texMarisa.getSize().y / 2.0f);
        sprite.setScale(80.0f / texMarisa.getSize().x, 80.0f / texMarisa.getSize().y);
    }

    void update(float px, float py, vector<Bullet*>& enemy_bullets) {
        timer++;
        if (hp > 1500) phase = 1;
        else if (hp > 1000) phase = 2;
        else if (hp > 500) phase = 3;
        else phase = 4;

        if (phase == 1) x = (PLAY_WIDTH / 2.0f) + sin(timer / 30.0f) * 140.0f;
        else if (phase == 2) x = (PLAY_WIDTH / 2.0f) + sin(timer / 40.0f) * 180.0f;
        else if (phase == 3) x = (PLAY_WIDTH / 2.0f) + sin(timer / 20.0f) * 100.0f;
        else { x += ((PLAY_WIDTH / 2.0f) - x) * 0.05f; y += (120.0f - y) * 0.05f; }

        if (phase == 1) {
            if (timer % 40 == 0) for (int i = 0; i < 10; i++) enemy_bullets.push_back(new EnemyBullet(x, y, i * (3.14159f * 2 / 10) + (timer * 0.1f), 3.0f));
            if (timer % 60 == 0) enemy_bullets.push_back(new EnemyBullet(x, y, atan2(py - y, px - x), 6.0f));
        }
        else if (phase == 2) {
            if (timer % 4 == 0) enemy_bullets.push_back(new EnemyGravityBullet(x, y, (rand() % 360) * 3.14159f / 180.0f, 5.0f + (rand() % 5) / 1.0f));
        }
        else if (phase == 3) {
            if (timer % 60 == 0) {
                float ang = atan2(py - y, px - x);
                enemy_bullets.push_back(new EnemyExplosiveBullet(x, y, ang - 0.2f, 5.0f));
                enemy_bullets.push_back(new EnemyExplosiveBullet(x, y, ang + 0.2f, 5.0f));
            }
        }
        else if (phase == 4) {
            laser_timer++;
            if (laser_timer < 120) {
                laser_active = false;
                if (laser_timer % 15 == 0) for (int i = 0; i < 32; i++) enemy_bullets.push_back(new EnemyBullet(x, y, i * (3.14159f * 2 / 32) + (laser_timer * 0.05f), 1.5f));
            }
            else if (laser_timer < 420) {
                laser_active = true;
                laser_rect = FloatRect(x - 75, y, 150, PLAY_HEIGHT);
                if (laser_timer % 4 == 0) {
                    for (int i = 0; i < 5; i++) {
                        enemy_bullets.push_back(new EnemyBullet(x, y, i * (3.14159f * 2 / 5) + (laser_timer * 0.06f), 5.5f));
                        enemy_bullets.push_back(new EnemyBullet(x, y, i * (3.14159f * 2 / 5) - (laser_timer * 0.06f), 4.0f));
                    }
                }
                if (laser_timer % 20 == 0) {
                    enemy_bullets.push_back(new EnemyGravityBullet(x, y, (rand() % 90) * 3.14159f / 180.0f, 3.0f + (rand() % 4)));
                    enemy_bullets.push_back(new EnemyGravityBullet(x, y, (90 + rand() % 90) * 3.14159f / 180.0f, 3.0f + (rand() % 4)));
                }
            }
            else if (laser_timer < 500) {
                laser_active = false;
                if (laser_timer == 421) {
                    for (int i = 0; i < 8; i++) enemy_bullets.push_back(new EnemyExplosiveBullet(x, y, i * (3.14159f * 2 / 8), 3.5f));
                }
            }
            else { laser_timer = 0; }
        }
    }

    void draw(RenderWindow& window) {
        if (phase == 4) {
            if (laser_timer > 0 && laser_timer < 120) {
                CircleShape warn(std::max(1.0f, 180.0f - laser_timer * 1.5f)); warn.setOrigin(warn.getRadius(), warn.getRadius()); warn.setPosition(x + PLAY_OFFSET_X, y + PLAY_OFFSET_Y); warn.setFillColor(Color::Transparent); warn.setOutlineColor(Color::Yellow); warn.setOutlineThickness(4); window.draw(warn);
                RectangleShape line(Vector2f(max(1.0f, laser_timer / 8.0f), PLAY_HEIGHT)); line.setFillColor(Color(255, 50, 50)); line.setOrigin(line.getSize().x / 2.0f, 0); line.setPosition(x + PLAY_OFFSET_X, y + PLAY_OFFSET_Y); window.draw(line);
            }
            else if (laser_active) {
                float jitter = (rand() % 12) - 6.0f;
                RectangleShape beamOuter(Vector2f(210, PLAY_HEIGHT)); beamOuter.setFillColor(Color(100, 255, 100)); beamOuter.setPosition(x - 105 + jitter + PLAY_OFFSET_X, y + PLAY_OFFSET_Y); window.draw(beamOuter);
                RectangleShape beamMid(Vector2f(150, PLAY_HEIGHT)); beamMid.setFillColor(Color(255, 255, 150)); beamMid.setPosition(x - 75 - jitter + PLAY_OFFSET_X, y + PLAY_OFFSET_Y); window.draw(beamMid);
                RectangleShape beamCore(Vector2f(60, PLAY_HEIGHT)); beamCore.setFillColor(Color(255, 255, 255)); beamCore.setPosition(x - 30 + jitter + PLAY_OFFSET_X, y + PLAY_OFFSET_Y); window.draw(beamCore);
            }
        }
        sprite.setPosition(x + PLAY_OFFSET_X, y + PLAY_OFFSET_Y);
        window.draw(sprite);

        RectangleShape bgBar(Vector2f(PLAY_WIDTH - 40, 6)); bgBar.setFillColor(Color(100, 100, 100)); bgBar.setPosition(20 + PLAY_OFFSET_X, 15 + PLAY_OFFSET_Y); window.draw(bgBar);
        if (hp > 0) {
            RectangleShape hpBar(Vector2f((float)hp / max_hp * (PLAY_WIDTH - 40), 6));
            hpBar.setFillColor(phase == 4 ? Color(255, 50, 50) : (phase == 3 ? Color(255, 200, 50) : Color(50, 255, 50)));
            hpBar.setPosition(20 + PLAY_OFFSET_X, 15 + PLAY_OFFSET_Y); window.draw(hpBar);
        }
    }
};

class Player {
public:
    float x, y, hit_radius;
    int lives, bombs, invincible_timer, shoot_timer;
    bool focus_mode;
    Sprite sprite;

    Player() {
        x = PLAY_WIDTH / 2.0f; y = PLAY_HEIGHT - 80.0f; hit_radius = 5.0f;
        lives = 99999; bombs = 2; invincible_timer = 0; shoot_timer = 0; focus_mode = false;
        sprite.setTexture(texReimu);
        sprite.setOrigin(texReimu.getSize().x / 2.0f, texReimu.getSize().y / 2.0f);
        sprite.setScale(64.0f / texReimu.getSize().x, 64.0f / texReimu.getSize().y);
    }

    void update(vector<Bullet*>& p_bullets) {
        focus_mode = Keyboard::isKeyPressed(Keyboard::LShift) || Keyboard::isKeyPressed(Keyboard::RShift) || Keyboard::isKeyPressed(Keyboard::LControl) || Keyboard::isKeyPressed(Keyboard::C);

        float speed = focus_mode ? 3.5f : 8.5f;
        if (Keyboard::isKeyPressed(Keyboard::Up) && y > 25) y -= speed;
        if (Keyboard::isKeyPressed(Keyboard::Down) && y < PLAY_HEIGHT - 25) y += speed;
        if (Keyboard::isKeyPressed(Keyboard::Left) && x > 25) x -= speed;
        if (Keyboard::isKeyPressed(Keyboard::Right) && x < PLAY_WIDTH - 25) x += speed;

        if (shoot_timer > 0) shoot_timer--;
        if (Keyboard::isKeyPressed(Keyboard::Z) && shoot_timer == 0) {
            sndShoot.play();
            if (focus_mode) {
                p_bullets.push_back(new PlayerBullet(x - 12, y));
                p_bullets.push_back(new PlayerBullet(x + 12, y));
            }
            else {
                p_bullets.push_back(new PlayerHomingBullet(x, y, -4.5f, -12.0f));
                p_bullets.push_back(new PlayerHomingBullet(x, y, 4.5f, -12.0f));
            }
            shoot_timer = 5;
        }
        if (invincible_timer > 0) invincible_timer--;
    }

    void draw(RenderWindow& window) {
        if (invincible_timer > 0 && (invincible_timer / 5) % 2 == 0) return;
        sprite.setPosition(x + PLAY_OFFSET_X, y + PLAY_OFFSET_Y);
        window.draw(sprite);
        if (focus_mode) {
            CircleShape core(hit_radius); core.setOrigin(hit_radius, hit_radius);
            core.setFillColor(Color(255, 255, 255)); core.setOutlineColor(Color::Black); core.setOutlineThickness(1);
            core.setPosition(x + PLAY_OFFSET_X, y + PLAY_OFFSET_Y); window.draw(core);
        }
    }
};

// ================= 4. 主程序 =================
int main() {
    RenderWindow window(VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "Touhou Clone - The Ultimate Stable Edition");
    window.setFramerateLimit(60);

    // 资源加载
    globalFont.loadFromFile("font.ttf");

    texReimu.loadFromFile("reimu.png");
    texMarisa.loadFromFile("marisa.png");
    texGlobalBg.loadFromFile("sakuya_bg.png");
    texPauseBg.loadFromFile("pause_bg.png");

    Image imgReimuSheet; imgReimuSheet.loadFromFile("reimu_sheet.png");
    texPlayerStraight.loadFromImage(imgReimuSheet, IntRect(82, 146, 14, 14));
    texPlayerHoming.loadFromImage(imgReimuSheet, IntRect(1, 148, 14, 9));

    Image imgStar; imgStar.loadFromFile("marisa_effects.png"); imgStar.createMaskFromColor(Color::Black); texStar.loadFromImage(imgStar, IntRect(1015, 264, 64, 62));
    Image imgMenu; imgMenu.loadFromFile("menu_sheet.png"); imgMenu.createMaskFromColor(Color::Black); texMenu.loadFromImage(imgMenu, IntRect(2, 513, 507, 222));
    Image imgGO; imgGO.loadFromFile("gameover_sheet.png"); imgGO.createMaskFromColor(Color::White); texGameover.loadFromImage(imgGO, IntRect(0, 59, 188, 73));

    if (bufShoot.loadFromFile("001.wav")) sndShoot.setBuffer(bufShoot);
    if (bufHit.loadFromFile("002.wav")) sndHit.setBuffer(bufHit);
    if (bufPichun.loadFromFile("003.wav")) sndPichun.setBuffer(bufPichun);
    if (bufBomb.loadFromFile("002.wav")) sndBomb.setBuffer(bufBomb);
    bgm.openFromFile("bgm.mp3"); bgm.setLoop(true); bgm.play();

    Player player; Boss boss;
    vector<Bullet*> player_bullets; vector<Bullet*> enemy_bullets;
    string gameState = "menu";
    bool isPaused = false;
    int bomb_timer = 0; float bomb_x = 0, bomb_y = 0;

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
                    gameState = "playing";

                    // 【修复2&1】：防止重置后仍然处于消弹状态，并且让 BGM 重生！
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
                    // N键无伤速通测试功能
                    if (event.key.code == Keyboard::N && !isPaused) {
                        boss.hp -= 500;
                        if (boss.hp <= 0) { gameState = "victory"; bgm.stop(); }
                    }
                }
            }
        }

        if (gameState == "playing" && !isPaused) {
            player.update(player_bullets);
            boss.update(player.x, player.y, enemy_bullets);

            if (bomb_timer > 0) { bomb_timer--; for (auto b : enemy_bullets) delete b; enemy_bullets.clear(); }

            for (auto b : player_bullets) {
                b->update(boss.x, boss.y, nullptr);
                if (b->active && sqrt(pow(b->x - boss.x, 2) + pow(b->y - boss.y, 2)) < boss.radius) {
                    boss.hp -= 1; sndHit.play(); b->active = false;
                    if (boss.hp <= 0) {
                        gameState = "victory"; bgm.stop();
                        break; // 【修复3】：防止鞭尸导致的逻辑死循环！
                    }
                }
            }

            bool hit = false;
            vector<Bullet*> current_enemy_bullets = enemy_bullets;
            for (auto b : current_enemy_bullets) {
                b->update(player.x, player.y, &enemy_bullets);
                if (b->active && sqrt(pow(b->x - player.x, 2) + pow(b->y - player.y, 2)) < player.hit_radius + 6) hit = true;
            }

            if (boss.phase == 4 && boss.laser_active && boss.laser_rect.contains(player.x, player.y)) hit = true;
            if (hit && player.invincible_timer <= 0) {
                player.lives--; sndPichun.play(); player.invincible_timer = 120;
                for (auto b : enemy_bullets) delete b; enemy_bullets.clear();
                if (player.lives <= 0) { gameState = "game_over"; bgm.stop(); }
            }

            player_bullets.erase(remove_if(player_bullets.begin(), player_bullets.end(), [](Bullet* b) { if (!b->active) { delete b; return true; } return false; }), player_bullets.end());
            enemy_bullets.erase(remove_if(enemy_bullets.begin(), enemy_bullets.end(), [](Bullet* b) { if (!b->active) { delete b; return true; } return false; }), enemy_bullets.end());
        }

        window.clear(Color::Black);

        Sprite bgGlobal(texGlobalBg);
        bgGlobal.setScale(1280.0f / texGlobalBg.getSize().x, 720.0f / texGlobalBg.getSize().y);
        window.draw(bgGlobal);

        if (gameState == "menu") {
            Sprite sMenu(texMenu); sMenu.setOrigin(texMenu.getSize().x / 2.0f, texMenu.getSize().y / 2.0f);
            sMenu.setScale(608.0f / texMenu.getSize().x, 266.0f / texMenu.getSize().y);
            sMenu.setPosition(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f - 100); window.draw(sMenu);
            drawOutlinedText(window, "Press [ENTER] to Start", 50, Color::White, SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f + 150, true);
        }
        else {
            RectangleShape playArea(Vector2f(PLAY_WIDTH, PLAY_HEIGHT)); playArea.setPosition(PLAY_OFFSET_X, PLAY_OFFSET_Y); playArea.setFillColor(Color::Black); window.draw(playArea);
            boss.draw(window); player.draw(window);
            for (auto b : player_bullets) b->draw(window);
            for (auto b : enemy_bullets) b->draw(window);
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
            float uiX = PLAY_OFFSET_X + PLAY_WIDTH + 60;

            drawOutlinedText(window, "Lives: " + to_string(player.lives), 50, player.lives > 3 ? Color(255, 100, 100) : Color::White, uiX, 150);
            drawOutlinedText(window, "Bombs: " + to_string(player.bombs), 50, Color(100, 255, 100), uiX, 220);
            drawOutlinedText(window, "Boss HP: " + to_string(boss.hp), 50, Color(255, 255, 100), uiX, 320);
        }
        window.display();
    }
    return 0;
}
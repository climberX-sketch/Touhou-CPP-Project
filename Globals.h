#pragma once // 防止重复包含，非常重要！
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <string>

// 常量
const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;
const int PLAY_WIDTH = 600;
const int PLAY_HEIGHT = 660;
const int PLAY_OFFSET_X = 40;
const int PLAY_OFFSET_Y = 30;

// 使用 extern 告诉编译器：“这些变量在别的地方定义了，大家共享使用”
extern sf::Texture texReimu, texMarisa, texStar, texPlayerStraight, texPlayerHoming, texGlobalBg, texPauseBg, texMenu, texGameover;
extern sf::Texture texCutin, texSpellBg;
extern sf::Texture texBflyPurple, texBflyBlue, texBflyRed, texStarBlue, texStarPurple, texLaserBody, texLaserBorder;
extern sf::Texture texItemPower;

extern sf::Font globalFont;
extern sf::SoundBuffer bufShoot, bufHit, bufPichun, bufBomb;
extern sf::Sound sndShoot, sndHit, sndPichun, sndBomb;
extern sf::Music bgm;

// 辅助函数声明
void drawOutlinedText(sf::RenderWindow& window, std::string str, int size, sf::Color col, float x, float y, bool centered = false);

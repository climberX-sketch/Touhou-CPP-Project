// v1.0版：初始化游戏项目，搭建基础视窗，导入核心图片与音频素材
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Window.hpp>
#include <iostream>

using namespace std;
using namespace sf;

int main() {
    // 1. 创建基础视窗
    RenderWindow window(VideoMode(1280, 720), "Touhou Clone - v1.0");
    window.setFramerateLimit(60);

    // 2. 加载核心资源 (测试路径是否正确)
    Texture bgTexture;
    bgTexture.loadFromFile("sakuya_bg.png");
    Sprite bgSprite(bgTexture);
    bgSprite.setScale(1280.0f / bgTexture.getSize().x, 720.0f / bgTexture.getSize().y);

    Music bgm;
    bgm.openFromFile("bgm.mp3");
    bgm.setLoop(true);
    bgm.play();

    // 3. 基础游戏循环
    while (window.isOpen()) {
        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed) window.close();
        }
        window.clear();
        window.draw(bgSprite); // 目前只渲染了背景
        window.display();
    }
    return 0;
}
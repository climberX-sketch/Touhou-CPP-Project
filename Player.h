#pragma once
#include "Globals.h"
#include "Bullets.h"
#include <vector>

class Player {
public:
    float x, y, hit_radius;
    int lives, bombs, invincible_timer, shoot_timer;
    bool focus_mode;
    sf::Sprite sprite;

    int score; // <--- 【在此处加上这一行！】
    int graze; // <--- 【在此处加上这一行！】
    int power;

    Player();
    void update(std::vector<Bullet*>& p_bullets);
    void draw(sf::RenderWindow& window);
};

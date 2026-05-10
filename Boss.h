#pragma once
#include "Globals.h"
#include "Bullets.h"
#include <vector>
#include <string>

class Boss {
public:
    float x, y, radius;
    int hp, max_hp, phase, timer, laser_timer;
    bool laser_active;
    sf::FloatRect laser_rect;
    sf::Sprite sprite;

    int spell_timer;
    std::string spell_name;

    Boss();
    void update(float px, float py, std::vector<Bullet*>& enemy_bullets);
    void draw(sf::RenderWindow& window);
};

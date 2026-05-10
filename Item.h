#pragma once
#include "Globals.h"

class Item {
public:
    float x, y, vx, vy;
    bool active;
    sf::Sprite sprite;

    Item(float startX, float startY);
    void update(float player_x, float player_y);
    void draw(sf::RenderWindow& window);
};
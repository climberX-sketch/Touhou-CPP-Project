#include "Globals.h"

sf::Texture texReimu, texMarisa, texStar, texPlayerStraight, texPlayerHoming, texGlobalBg, texPauseBg, texMenu, texGameover;
sf::Texture texCutin, texSpellBg;
sf::Texture texBflyPurple, texBflyBlue, texBflyRed, texStarBlue, texStarPurple, texLaserBody, texLaserBorder;
sf::Texture texItemPower;

sf::Font globalFont;
sf::SoundBuffer bufShoot, bufHit, bufPichun, bufBomb;
sf::Sound sndShoot, sndHit, sndPichun, sndBomb;
sf::Music bgm;

void drawOutlinedText(sf::RenderWindow& window, std::string str, int size, sf::Color col, float x, float y, bool centered) {
    sf::Text text(str, globalFont, size);
    text.setFillColor(col);
    text.setOutlineColor(sf::Color::Black);
    text.setOutlineThickness(2.0f);
    if (centered) {
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
    }
    text.setPosition(x, y);
    window.draw(text);
}
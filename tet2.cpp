#include <SFML/Graphics.hpp>
#include <array>
#include <vector>
#include <random>
#include <string>
#include <iostream>

using namespace std;

const int BLOCK_SIZE = 30;
const int BOARD_WIDTH = 10;
const int BOARD_HEIGHT = 20;

struct Point {
    int x, y;
};

class Tetromino {
public:
    vector<Point> blocks;
    int type;

    Tetromino(int t) : type(t) {
        // Shapes: only a couple for demo, expand later
        static const vector<vector<Point>> shapes = {
            {{0,0},{1,0},{0,1},{1,1}},   // O
            {{0,0},{1,0},{2,0},{3,0}},   // I
        };
        blocks = shapes[t];
    }

    void move(int dx, int dy) {
        for (auto &b : blocks) {
            b.x += dx;
            b.y += dy;
        }
    }
};

int main() {
    sf::RenderWindow window(
        sf::VideoMode({BOARD_WIDTH * BLOCK_SIZE, BOARD_HEIGHT * BLOCK_SIZE}),
        "Tetris (SFML 3.0.1)");

    sf::Font font;
    if (!font.openFromFile("DejaVuSans.ttf")) {
        cerr << "Font not found!\n";
        return 1;
    }

    sf::Text scoreText(font, "Score: 0", 20);
    scoreText.setPosition({5.f, 5.f});

    Tetromino current(0);
    int score = 0;

    while (window.isOpen()) {
        // Event handling (SFML 3 way)
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
            if (auto key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->code == sf::Keyboard::Key::Left) {
                    current.move(-1,0);
                } else if (key->code == sf::Keyboard::Key::Right) {
                    current.move(1,0);
                } else if (key->code == sf::Keyboard::Key::Down) {
                    current.move(0,1);
                }
            }
        }

        // Draw
        window.clear(sf::Color::Black);

        // Draw tetromino blocks
        sf::RectangleShape block({(float)BLOCK_SIZE-1, (float)BLOCK_SIZE-1});
        block.setFillColor(sf::Color::White);
        for (auto &b : current.blocks) {
            block.setPosition({(float)b.x * BLOCK_SIZE, (float)b.y * BLOCK_SIZE});
            window.draw(block);
        }

        window.draw(scoreText);
        window.display();
    }

    return 0;
}

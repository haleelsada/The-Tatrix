// tetris_sfml3_fixed.cpp
// compilation: g++ tet_editable.cpp -o ttemp -lsfml-graphics -lsfml-window -lsfml-system
#include <SFML/Graphics.hpp>
#include <array>
#include <optional>
#include <random>
#include <vector>
#include <chrono>
#include <iostream>

using namespace std;

struct Cell { int x, y; };

static constexpr int COLS = 10;
static constexpr int ROWS = 20;
static constexpr int TILE = 32;
static constexpr int BORDER = 4;
static constexpr int WIDTH  = COLS * TILE + BORDER * 2;
static constexpr int HEIGHT = ROWS * TILE + BORDER * 2;

// 7 Tetrominoes, each with 4 rotation states; encoded as 4x4 mini-grids flattened
// ie, each inner array represent one shape, with each item representing 4 orientations
static const array<array<string, 4>, 7> SHAPES {{
    // I
    { "....""####""....""....",
      "..#." "..#." "..#." "..#.",
      "....""....""####""....",
      ".#.." ".#.." ".#.." ".#.." },

    // J
    { "#.. ""### ""....""....",
      ".##."".#.."".#.." "....",
      "....""###." "..#." "....",
      ".#.."".#.." "##.." "...." },

    // L
    { "..#.""###.""....""....",
      ".#.."".#.."".##." "....",
      "....""###." "#..." "....",
      "##.."".#.."".#.." "...." },

    // O
    { ".##."".##." "....""....",
      ".##."".##." "....""....",
      ".##."".##." "....""....",
      ".##."".##." "....""...." },

    // S
    { ".##.""##.." "....""....",
      ".#.."".##." "..#." "....",
      "...."".##.""##.." "....",
      "#..." "##.." ".#.." "...." },

    // T
    { ".#.." "###." "....""....",
      ".#.." ".##." ".#.." "....",
      "...." "###." ".#.." "....",
      ".#.." "##.." ".#.." "...." },

    // Z
    { "##.." ".##." "....""....",
      "..#." ".##." ".#.." "....",
      "...." "##.." ".##." "....",
      ".#.." "##.." "#..." "...." }
}};

struct Piece {
    int type;      // 0..6
    int rotation;  // 0..3
    int x, y;      // grid position of 4x4 top-left
};

struct Game {
    array<array<int, COLS>, ROWS> board{}; // 0 empty, 1..7 filled color index
    Piece cur{};
    bool gameOver = false;
    int score = 0;
};

// allocating color to each shape and cell
static array<sf::Color, 8> COLORS = {
    sf::Color(0,0,0),            // empty
    sf::Color(0, 240, 240),      // I
    sf::Color(0, 0, 240),        // J
    sf::Color(240, 160, 0),      // L
    sf::Color(240, 240, 0),      // O
    sf::Color(0, 240, 0),        // S
    sf::Color(160, 0, 240),      // T
    sf::Color(240, 0, 0)         // Z
};

// Check empty cell, return true if cell is filled
// idx=index in row majored ordering
static bool cellFilled(const string& mask, int idx) {
    return mask[idx] != '.' && mask[idx] != ' ';
}
// Cell = struct of x,y coordinate
// Piece struct contain coordinate of top left cell, this function return an array of 
// positions of a Piece in cell if the cell is already filled
static vector<Cell> cellsFor(const Piece& p) {
    vector<Cell> out;
    const string& m = SHAPES[p.type][p.rotation];
    out.reserve(4);
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c) {
            int idx = r * 4 + c;
            if (cellFilled(m, idx)) out.push_back({p.x + c, p.y + r});
        }
    return out;
}

static bool inBounds(const Cell& c) {
    return c.x >= 0 && c.x < COLS && c.y < ROWS;
}

static bool canPlace(const Game& g, const Piece& p) {
    for (auto c : cellsFor(p)) {
        if (!inBounds(c) || (c.y >= 0 && g.board[c.y][c.x] != 0)) return false;
        // y < 0 (above top) is allowed while spawning
    }
    return true;
}

static void lockPiece(Game& g) {
    for (auto c : cellsFor(g.cur)) {
        if (c.y >= 0 && c.y < ROWS) g.board[c.y][c.x] = g.cur.type + 1;
        else g.gameOver = true; // stacked above top -> game over
    }
}

static int clearLines(Game& g) {
    int lines = 0;
    for (int r = ROWS - 1; r >= 0; --r) {
        bool full = true;
        for (int c = 0; c < COLS; ++c)
            if (g.board[r][c] == 0) { full = false; break; }
        if (full) {
            ++lines;
            for (int rr = r; rr > 0; --rr) g.board[rr] = g.board[rr - 1];
            g.board[0].fill(0);
            ++r; // re-check same row after pull-down
        }
    }
    static const int LINE_SCORES[5] = {0, 100, 300, 500, 800};
    g.score += LINE_SCORES[min(lines, 4)];
    return lines;
}

static Piece randomPiece(mt19937& rng) {
    uniform_int_distribution<int> dist(0, 6);
    Piece p{};
    p.type = dist(rng);
    p.rotation = 0;
    p.x = COLS / 2 - 2;
    p.y = -2; // start slightly above the top
    return p;
}

static void rotate(Piece& p, int dir) { // dir = +1 CW, -1 CCW
    p.rotation = (p.rotation + (dir > 0 ? 1 : 3)) & 3;
}

// Simple wall-kick, ie if a rotation is not possible we check if shifted rotation
// is possible or not, if return true and change Piece, pointer parameters passed
static bool tryRotate(const Game& g, Piece& p, int dir) {
    Piece test = p;
    rotate(test, dir);
    array<int, 5> kicks = {0, -1, 1, -2, 2};
    for (int dx : kicks) {
        Piece shifted = test;
        shifted.x += dx;
        if (canPlace(g, shifted)) { p = shifted; return true; }
    }
    return false;
}

static bool moveIf(Game& g, int dx, int dy) {
    Piece t = g.cur;
    t.x += dx; t.y += dy;
    if (canPlace(g, t)) { g.cur = t; return true; }
    return false;
}

// Heuristic scoring: lower is better for the player, higher is worse
static int evaluateBoard(const Game& g) {
    int holes = 0, height = 0;

    // Count holes and max height
    for (int c = 0; c < COLS; c++) {
        bool blockSeen = false;
        for (int r = 0; r < ROWS; r++) {
            if (g.board[r][c] != 0) {
                blockSeen = true;
                height = max(height, ROWS - r);
            } else if (blockSeen) {
                holes++; // empty below a block = hole
            }
        }
    }
    return height * 5 + holes * 10; // weighted score
}
// return no of lines(rows) that can be cleared
static int checkclearLines(Game& g) {
    int lines = 0;
    for (int r = ROWS - 1; r >= 0; --r) {
        bool full = true;
        for (int c = 0; c < COLS; ++c)
            if (g.board[r][c] == 0) { full = false; break; }
        if (full) {
            ++lines;
        }
    }
    return lines;
}

// Heuristic scoring: more is better for the player, opposite of prev one
static int evaluateBoard2(const Game& g) {
    // int holes = 0, height = 0;

    // Count holes and max height
    // for (int c = 0; c < COLS; c++) {
    //     bool blockSeen = false;
    //     for (int r = 0; r < ROWS; r++) {
    //         if (g.board[r][c] != 0) {
    //             blockSeen = true;
    //             height = max(height, ROWS - r);
    //         } else if (blockSeen) {
    //             holes++; // empty below a block = hole
    //         }
    //     }
    // }
    Game gtemp = g;
    return checkclearLines(gtemp);
}

static Piece worstPiece(const Game& g) {
    Piece worst{};
    int worstScore = -1;
    int bestScore = INT8_MAX;

    // Try all 7 shapes
    for (int t = 0; t < 7; t++) {
        for (int rot = 0; rot < 4; rot++) {
            Piece candidate;
            candidate.type = t;
            candidate.rotation = rot;

            // Try all x positions
            for (int x = -2; x < COLS; x++) {
                candidate.x = x;
                candidate.y = -2;

                // Drop piece down as far as possible
                while (canPlace(g, candidate)) candidate.y++;
                candidate.y--; // last valid position

                if (!canPlace(g, candidate)) continue;

                // Copy board and lock piece
                Game test = g;
                test.cur = candidate;
                lockPiece(test);
                //clearLines(test);

                // returning hueristics that are not in favour of the player
                // int score = evaluateBoard(test);

                // in favour of the player
                int score = evaluateBoard2(test);

                if (score > worstScore) {
                    worstScore = score;
                    worst = candidate;
                }
            }
        }
    }
    cout << "choose " << worst.type << " with score " << worstScore << endl;
    worst.rotation = 0;
    worst.x = COLS / 2 - 2;
    worst.y = -2;
    return worst;
}


int main() {
    sf::RenderWindow window(sf::VideoMode({unsigned(WIDTH), unsigned(HEIGHT)}), "Tetris");
    // window getting updated 60 times per s
    window.setFramerateLimit(60);

    // Tiles
    sf::RectangleShape tile(sf::Vector2f(float(TILE - 1), float(TILE - 1)));

    // RNG
    mt19937 rng(uint32_t(chrono::steady_clock::now().time_since_epoch().count()));

    Game game;
    game.cur = randomPiece(rng);

    // Font (SFML 3 uses openFromFile)
    sf::Font font;
    bool haveFont = font.openFromFile("/home/haleelsada/Downloads/IITD/COURSE/AI/tetris/DejaVuSerifCondensed-BoldItalic.ttf"); // change path as needed

    // Pre-create texts (SFML 3 removed default ctor for sf::Text)
    sf::Text scoreText(font, "", 18);
    scoreText.setFillColor(sf::Color(230, 230, 240));
    scoreText.setPosition(sf::Vector2f(8.f, 6.f));

    sf::Clock frameClock;

    // time since current Piece moved one row down
    float dropTimer = 0.f;

    // time the piece wait to go down one row
    float dropInterval = 0.6f;

    float softTimer = 0.f;

    while (window.isOpen()) {
        // --- Timing ---
        // dt is time since last restart
        float dt = frameClock.restart().asSeconds();
        dropTimer += dt;

        // --- Events (std::optional + getIf in SFML 3) ---
        // loop continues as long as an event is pending
        while (auto ev = window.pollEvent()) {
            if (ev->is<sf::Event::Closed>()) window.close();

            if (!game.gameOver) {
                if (auto key = ev->getIf<sf::Event::KeyPressed>()) {
                    auto sc = key->scancode; // Scancode enum in SFML 3
                    if (sc == sf::Keyboard::Scancode::Escape) window.close();
                    else if (sc == sf::Keyboard::Scancode::Up)    tryRotate(game, game.cur, +1);
                    else if (sc == sf::Keyboard::Scancode::Left)  moveIf(game, -1, 0);
                    else if (sc == sf::Keyboard::Scancode::Right) moveIf(game, +1, 0);
                    else if (sc == sf::Keyboard::Scancode::Space) {
                        while (moveIf(game, 0, +1)) {}
                        lockPiece(game);
                        clearLines(game);
                        game.cur = worstPiece(game);
                        if (!canPlace(game, game.cur)) game.gameOver = true;
                        dropTimer = 0.f;
                    }
                }
            } else {
                if (auto key = ev->getIf<sf::Event::KeyPressed>()) {
                    if (key->scancode == sf::Keyboard::Scancode::Escape) window.close();
                }
            }
        }

        // Continuous soft drop
        if (!game.gameOver && sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Down)) {
            softTimer += dt;
            if (softTimer > 0.03f) {
                if (!moveIf(game, 0, +1)) softTimer = 0.f;
                else softTimer = 0.f;
            }
        } else {
            softTimer = 0.f;
        }

        // Gravity with simple speedup
        dropInterval = max(0.4f, 0.6f - float(game.score) * 0.0005f);
        // going down one row and checking if game over
        if (!game.gameOver && dropTimer >= dropInterval) {
            dropTimer = 0.f;
            if (!moveIf(game, 0, +1)) {
                lockPiece(game);
                clearLines(game);
                game.cur = worstPiece(game);
                if (!canPlace(game, game.cur)) game.gameOver = true;
            }
        }

        // --- Render ---
        window.clear(sf::Color(20, 20, 30));

        // clearing the board, then filling the available shape colors
        // then grid lines and last current shape
        // Placed blocks
        for (int r = 0; r < ROWS; ++r) {
            for (int c = 0; c < COLS; ++c) {
                int v = game.board[r][c];
                if (v == 0) continue;
                tile.setFillColor(COLORS[v]);
                tile.setPosition(sf::Vector2f(float(BORDER + c * TILE), float(BORDER + r * TILE)));
                window.draw(tile);
            }
        }

        // Current piece
        if (!game.gameOver) {
            tile.setFillColor(COLORS[game.cur.type + 1]);
            for (auto cell : cellsFor(game.cur)) {
                if (cell.y < 0) continue;
                tile.setPosition(sf::Vector2f(float(BORDER + cell.x * TILE), float(BORDER + cell.y * TILE)));
                window.draw(tile);
            }
        }

        // Grid
        sf::RectangleShape gridLine;
        gridLine.setFillColor(sf::Color(60, 60, 80));
        for (int c = 0; c <= COLS; ++c) {
            gridLine.setSize(sf::Vector2f(1.f, float(ROWS * TILE)));
            gridLine.setPosition(sf::Vector2f(float(BORDER + c * TILE), float(BORDER)));
            window.draw(gridLine);
        }
        for (int r = 0; r <= ROWS; ++r) {
            gridLine.setSize(sf::Vector2f(float(COLS * TILE), 1.f));
            gridLine.setPosition(sf::Vector2f(float(BORDER), float(BORDER + r * TILE)));
            window.draw(gridLine);
        }

        // Border
        sf::RectangleShape borderRect(sf::Vector2f(float(COLS * TILE + 2), float(ROWS * TILE + 2)));
        borderRect.setPosition(sf::Vector2f(float(BORDER - 1), float(BORDER - 1)));
        borderRect.setFillColor(sf::Color::Transparent);
        borderRect.setOutlineThickness(2.f);
        borderRect.setOutlineColor(sf::Color(140, 140, 180));
        window.draw(borderRect);

        // UI text
        if (haveFont) {
            scoreText.setString("Score: " + to_string(game.score));
            window.draw(scoreText);

            if (game.gameOver) {
                sf::Text over(font, "GAME OVER - Press Esc", 24);
                over.setFillColor(sf::Color(255, 200, 200));
                over.setPosition(sf::Vector2f(12.f, float(HEIGHT / 2 - 16)));
                window.draw(over);
            }
        }

        window.display();
    }
    return 0;
}

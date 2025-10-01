# The Tatrix

<p align="center">
  <img src="https://i.pinimg.com/736x/22/67/e7/2267e7fe7037001c4ec3acb6e8455d58.jpg"/>
</p>

This is a Tetris game implemented in C++ with two AI modes.

## Features
- Classic Tetris gameplay with line clearing and scoring.

- Heuristic-based AI that can evaluate board states.

## setup

Install sfml and c++ if not already installed

```
git clone [https://github.com/your-username/tetris-ai.git](https://github.com/haleelsada/The-Tatrix.git
cd The-Tatrix
```
compile and run.
for normal game
```
g++ tet.cpp -o tetris -lsfml-graphics -lsfml-window -lsfml-system
./tetris
```

for best piece game 
```
g++ tet_v1.cpp -o tetris -lsfml-graphics -lsfml-window -lsfml-system
./tetris
```

for worst piece game
```
g++ tet_v2.cpp -o tetris -lsfml-graphics -lsfml-window -lsfml-system
./tetris
```

## Heuristic Function

The AI evaluates each board state with a heuristic scoring function.

It considers three main factors for every possible moves:

- Lines Cleared – clearing more lines is rewarded (+4 points per line).

- Board Height – higher stacks are penalized (–1 per extra height).

- Holes – empty spaces below blocks are heavily penalized (–2 per hole).

Score formula:
```
score = (lines_cleared * 4) - (delta_height) - (holes * 2)
```

Best Move agent chooses the placement with the highest score.

Worst Move aget chooses the shape whose best possible placement has the lowest score compared to other shapes.

This balance makes the AI mimic how humans judge a "good" or "bad" position in Tetris.


## What's next?
- Add game playing agent
- Implement reinforcement learning instead of hardcoding hueristics
- Features to improve board
- Host in a server and leaderboard
- Add two level search instead of one level.

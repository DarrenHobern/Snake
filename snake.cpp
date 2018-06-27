/*
Darren Hobern
2018
Snake game with mostly accurate rules.

Pickup can spawn ontop of the snake, and if its late game then you just lose.
Game doesn't speed up.
*/


#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <tuple>
#include <ncurses.h>
#include <sys/mman.h>
#include <string.h>

#ifndef B_SIZE
#define B_SIZE 10
#endif

std::tuple<int, int> directions[] = {{ 0, -1},  // Up
                                     { 0,  1},  // Down
                                     { 1,  0},  // Right
                                     {-1,  0}}; // Left

// ----------------------------------------------------------------------------
// PICKUPS
class Pickup {
    public:
        Pickup( void );
        void move( void );
        static int x;
        static int y;
};

Pickup::Pickup( void ) {
    move();
}

int Pickup::x = 0;
int Pickup::y = 0;

void Pickup::move( void ) {
    // TODO dont spawn where the snake is
    x = rand()%B_SIZE;
    y = rand()%B_SIZE;
}
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Snake Segment
class SnakeSegment {
    public:
        SnakeSegment( int, int );
        int x, y;
        SnakeSegment *prev;
        SnakeSegment *next;
};

SnakeSegment::SnakeSegment( int xpos, int ypos ) {
    x = xpos;
    y = ypos;
}
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// SNAKE? SNAAAAAAAKE!
class Snake {
    public:
        Snake( int, int );
        void checkForFood( Pickup );
        bool move( Pickup );
        bool checkDeath( void );
        int countSnake ( void );
        SnakeSegment *head;
        SnakeSegment *tail;
        std::tuple<int,int> direction = directions[2];  // right
        bool justAte = false;
        bool dead = false;

};

Snake::Snake(int x, int y) {
    SnakeSegment *s = new SnakeSegment(x,y);
    head = s;
    tail = s;
}

void Snake::checkForFood( Pickup p ) {
    // if the tail is in the same position as the pickup eat it and spawn new segment
    // TODO move the pickup when the head collides, but keep segment at tail.
    if (tail->x == p.x && tail->y == p.y) {
        SnakeSegment* newTail = new SnakeSegment(p.x, p.y);
        tail->next = newTail;
        newTail->next = NULL;
        newTail->prev = tail;
        justAte = true;
        tail = newTail;
        p.move();
    }
}

/*
 * Returns true if the snake is dead.
 * Move the snake by iterating over each segment tail >> head moving each
 * segment to the position of the segment in front of it. Except the head
 * which has the direction added to it's position.
 * Also check for collision with itself (dead) or the pickup (points).
 */
bool Snake::move( Pickup p ) {

    int newX = (B_SIZE+( (head->x + std::get<0>(direction))%B_SIZE )) % B_SIZE;
    int newY = (B_SIZE+( (head->y + std::get<1>(direction))%B_SIZE )) % B_SIZE;

    SnakeSegment *snakeItr = tail;
    while(snakeItr != head) {
        if (justAte && snakeItr == tail) {
            justAte = false;
            snakeItr = snakeItr->prev;
            continue;
        }
        snakeItr->x = snakeItr->prev->x;
        snakeItr->y = snakeItr->prev->y;
        snakeItr = snakeItr->prev;
    }

    head->x = newX;
    head->y = newY;
    if (checkDeath())
        return true;
    checkForFood(p);
    return false;
}

/**
 * Return true if the snake has hit itself
 */
bool Snake::checkDeath( void ) {
    SnakeSegment *snakeItr = head->next;
    while(snakeItr) {
        if (head->x == snakeItr->x && head->y == snakeItr->y) {
            return true;
        }
        snakeItr = snakeItr->next;
    }
    return false;
}

/*
 * Returns the number of segments in the snake.
 */
int Snake::countSnake( void ) {
    int count = 0;
    SnakeSegment *snakeItr = head;
    while(snakeItr) {
        ++count;
        snakeItr = snakeItr->next;
    }
    return count;
}
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// GAME THINGS

/*
 * Processes user input to change direction or exit the game.
 */
void processInput( Snake *snake ) {
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    int ch;
    if ((ch = getch()) == ERR){}
    else {
        if (snake->dead) {
            return;
        }
        switch (ch) {
            case KEY_UP:
                snake->direction = directions[0];
                break;
            case KEY_DOWN:
                snake->direction = directions[1];
                break;
            case KEY_RIGHT:
                snake->direction = directions[2];
                break;
            case KEY_LEFT:
                snake->direction = directions[3];
                break;
            default:
                snake->dead = true;
                break;
        }
    }
}

/*
 * Prints the board on the window with some sweet ascii art.
 */
void printBoard(WINDOW *win, int **board, Snake *s, Pickup p) {
    SnakeSegment *snakeItr = s->head;
    while(snakeItr) {
        board[snakeItr->y][snakeItr->x] = 1;
        snakeItr = snakeItr->next;
    }
    board[p.y][p.x] = 2;

    for (int r = 0; r < B_SIZE; r++) {
        for (int c = 0; c < B_SIZE; c++) {
            if (board[r][c] == 1) {     // segment
                waddstr(win, "# ");
            }
            else if (board[r][c] == 2){ // pickup
                waddstr(win, "@ ");
            }
            else {
                waddstr(win, "- ");     // empty space
            }
            board[r][c] = 0; // clear the board for next time
        }
    }
    wmove(win, 0,0);  // reset cursor position
    wrefresh(win);
}

/*
 * Displays the player's score and waits for any key.
 */
void gameOver(int score) {
    const char *message = "GAME OVER! Your Score: %d";
    move(LINES-1, COLS/2 - strlen(message)/2);
    printw(message, score);
    refresh();
    nodelay(stdscr, FALSE);
    getch();
}

/*
 * Returns true if the game has ended.
 * Takes input from the user (nodely is set so it doesn't block)
 * Moves the snake one step.
 * Prints the game state.
 */
bool gameStep(WINDOW *win, int **board, Snake *snake, Pickup p) {
    processInput(snake);
    if (snake->move(p))
        return true;
    printBoard(win, board, snake, p);
    return false;
}

int main(int argc, char const *argv[]) {
    srand(time(NULL));

    // Curses Setup
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    WINDOW *win = newwin(B_SIZE, B_SIZE*2, LINES/2-B_SIZE/2, COLS/2-B_SIZE);

    // 2d representation of the board - for printing purposes
    int **board = new int*[B_SIZE];
    for (int r = 0; r < B_SIZE; r++) {
        board[r] = new int[B_SIZE];
    }

    Snake *snake = new Snake(B_SIZE/2, B_SIZE/2);
    Pickup p;

    // Play the game
    while(true) {
        if (gameStep(win, board, snake, p))
            break;
        sleep(1);
    }
    // Game over
    delwin(win);
    gameOver(snake->countSnake());

    clear();
    refresh();
    endwin(); // <-- Do this else ncurses will curse your terminal
    return 0;
}

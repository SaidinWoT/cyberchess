#include <ncurses.h>
#include "chess.h"

#define COLOR(file,rank) (((file) + (rank)) % 2 ? 1 : 2)

void printSpot(Pos spot, Game *game);
void printBoard(Game *game);
void user(Game *game);

int main(int argc, char **argv) {
    initscr();
    if(has_colors()) {
        start_color();
        init_pair(1, COLOR_BLACK, COLOR_WHITE);
        init_pair(2, COLOR_WHITE, COLOR_BLACK);
        init_pair(3, COLOR_CYAN, COLOR_BLACK);
    }
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);
    noecho();

    Game *game = newGame();
    printBoard(game);

    user(game);
    endwin();

    return 0;
}

void printSpot(Pos spot, Game *game) {
    static char *reps = REPS;
    attron(COLOR_PAIR(COLOR(spot.file,spot.rank)));
    mvprintw(7-spot.rank, 3*spot.file, " %c ", reps[value(spot, game)]);
    attroff(COLOR_PAIR(COLOR(spot.file,spot.rank)));
}

void printBoard(Game *game) {
    Pos iter;
    for(iter.rank = 0; iter.rank >= 0; ++iter.rank) {
        for(iter.file = 0; iter.file >= 0; ++iter.file) {
            printSpot(iter, game);
        }
    }
    mvprintw(8, 0, "%X %X %X", game->info.castle, game->info.enp, game->info.color);
}

void user(Game *game) {
    static char ch;
    static Move move;
    static char cursX = 0;
    static char cursY = 0;
    mvchgat(cursY, 3*cursX, 3, A_REVERSE, 3, NULL);
    for(ch = getch(); ch != 'q'; ch = getch()) {
        mvchgat(cursY, 3*cursX, 3, A_NORMAL, COLOR(7-cursY,cursX), NULL);
        switch(ch) {
            case 'k':
                cursY = (cursY + 7) % 8;
                break;
            case 'j':
                cursY = (cursY + 1) % 8;
                break;
            case 'h':
                cursX = (cursX + 7) % 8;
                break;
            case 'l':
                cursX = (cursX + 1) % 8;
                break;
            case 's':
                move.src = (Pos){cursX, 7-cursY};
                move.piece = value(move.src, game);
                break;
            case 'd':
                move.dst = (Pos){cursX, 7-cursY};
                move.capture = value(move.dst, game);
                execMove(move, game);
                printBoard(game);
                break;
        }
        mvchgat(cursY, 3*cursX, 3, A_REVERSE, 3, NULL);
    }
}

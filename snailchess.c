#include <ncurses.h>
#include "chess.h"

#define COLOR(file,rank) (((file) + (rank)) % 2 ? 1 : 2)

void printSpot(Pos spot, Game *game);
void printBoard(Game *game);
void user(Game *game);
char getPromo();

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

    Game *game = newGame(getPromo);
    printBoard(game);

    user(game);
    endwin();

    return 0;
}

char getPromo() {
    char piece;
    mvprintw(9, 0, "What would you like to promote your pawn to?");
    mvprintw(10, 0, "R N B Q");
    piece = getch();
    switch(piece) {
        case 'Q':
        case 'q':
            return QUEEN;
        case 'N':
        case 'n':
            return KNIGHT;
        case 'R':
        case 'r':
            return ROOK;
        case 'B':
        case 'b':
            return BISHOP;
    }
}

void printSpot(Pos spot, Game *game) {
    static char *reps = REPS;
    attron(COLOR_PAIR(COLOR(spot.file,spot.rank)));
    mvprintw(7-spot.rank, 3*spot.file, " %c ", reps[value(spot, game)]);
    attroff(COLOR_PAIR(COLOR(spot.file,spot.rank)));
}

void printCap(char color, char row, char spot, Game *game) {
    static char *reps = REPS;
    mvprintw(spot, (color ? 25 : 32)+3*row, " %c ", reps[capval(color, row, spot, game)]);
}

void printBoard(Game *game) {
    Pos iter;
    move(8, 0);
    clrtobot();
    for(iter.rank = 0; iter.rank >= 0; ++iter.rank) {
        for(iter.file = 0; iter.file >= 0; ++iter.file) {
            printSpot(iter, game);
        }
    }
    for(iter.rank = 0; iter.rank >=0; ++iter.rank) {
        printCap(0, 0, iter.rank, game);
        printCap(0, 1, iter.rank, game);
        printCap(1, 0, iter.rank, game);
        printCap(1, 1, iter.rank, game);
    }
    mvprintw(8, 0, "%X %X", game->info.castle, game->info.color);
}

void displayMoves(Move *move) {
    Move *iter;
    char n;
    if(move == NULL) {
        return;
    }
    for(iter = move; iter->next; iter = iter->next) {
        mvchgat(7-iter->dst.rank, 3*iter->dst.file, 3, A_REVERSE, 3, NULL);
    }
}

void user(Game *game) {
    static char src = 1;
    static char ch;
    static char ret;
    static Move *valid;
    static Move move;
    static Pos pos;
    static char cursX = 0;
    static char cursY = 0;
    mvchgat(cursY, 3*cursX, 3, A_REVERSE, 3, NULL);
    for(ch = getch(); ch != 'q'; ch = getch()) {
        mvchgat(cursY, 3*cursX, 3, A_NORMAL, COLOR(7-cursY,cursX), NULL);
        switch(ch) {
            case 'w':
                cursY = (cursY + 7) % 8;
                break;
            case 's':
                cursY = (cursY + 1) % 8;
                break;
            case 'a':
                cursX = (cursX + 7) % 8;
                break;
            case 'd':
                cursX = (cursX + 1) % 8;
                break;
            case 'v':
                printBoard(game);
                pos = (Pos){cursX, 7-cursY};
                valid = possible(pos, game);
                displayMoves(valid);
            case ' ':
                if(src) {
                    move.src = (Pos){cursX, 7-cursY};
                    move.piece = value(move.src, game);
                    src = !src;
                } else {
                    move.dst = (Pos){cursX, 7-cursY};
                    move.capture = value(move.dst, game);
                    ret = execMove(move, game);
                    if(ret > 0) {
                        printBoard(game);
                    }
                    switch(ret) {
                        case TURN:
                            mvprintw(9, 0, "It's not your turn.");
                            break;
                        case INVALID:
                            mvprintw(9, 0, "That piece can't move like that.");
                            src = !src;
                            break;
                        case THREAT:
                            mvprintw(9, 0, "That would leave your king in check.");
                            src = !src;
                            break;
                        case CHECK:
                            mvprintw(9, 0, "Check!");
                            break;
                        case STALE:
                            mvprintw(9, 0, "Stalemate!");
                            break;
                        case MATE:
                            mvprintw(9, 0, "Checkmate!");
                            break;
                    }
                    src = !src;
                    clrtoeol;
                }
                break;
        }
        mvchgat(cursY, 3*cursX, 3, A_REVERSE, 3, NULL);
    }
}

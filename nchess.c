#include <ncurses.h>
#include "chess.h"

#define COLOR(file,rank) (((file) + (rank)) % 2 ? 1 : 2)
#define MSG 10

void printSpot(Pos spot, Game *game);
void printBorder();
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

    printBorder();
    Game *game = newGame(getPromo);
    printBoard(game);

    user(game);
    endwin();

    return 0;
}

char getPromo() {
    char piece;
    mvprintw(MSG, 0, "What would you like to promote your pawn to?");
    mvprintw(MSG+1, 0, "R N B Q");
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
    mvprintw(8-spot.rank, 3*(1+spot.file), " %c ", reps[value(spot, game)]);
    attroff(COLOR_PAIR(COLOR(spot.file,spot.rank)));
}

void printCap(char color, char row, char spot, Game *game) {
    static char *reps = REPS;
    mvprintw(1+spot, (color ? 31 : 36)+3*row, " %c ", reps[capval(color, row, spot, game)]);
}

void printBorder() {
    char i;
    for(i = 0; i < 8; ++i) {
        mvprintw(0, 3*i+4, "%c", 97+i);
        mvprintw(9, 3*i+4, "%c", 97+i);
        mvprintw(i+1, 1, "%d", 8-i);
        mvprintw(i+1, 28, "%d", 8-i);
    }
    mvprintw(0, 30, "Capture Zone");
}

void printBoard(Game *game) {
    Pos iter;
    move(10, 0);
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
}

void displayMoves(Move *move) {
    Move *iter;
    char n;
    if(move == NULL) {
        return;
    }
    for(iter = move; iter->next; iter = iter->next) {
        mvchgat(8-iter->dst.rank, 3*(1+iter->dst.file), 3, A_REVERSE, 3, NULL);
    }
}

void user(Game *game) {
    static char ch;
    static char ret;
    static Move *valid;
    static Move move;
    static Pos pos;
    static char cursX = 0;
    static char cursY = 0;
    mvchgat(1+cursY, 3*(1+cursX), 3, A_REVERSE, 3, NULL);
    for(ch = getch(); ch != 'q'; ch = getch()) {
        mvchgat(1+cursY, 3*(1+cursX), 3, A_NORMAL, COLOR(7-cursY,cursX), NULL);
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
            case 'v':
                printBoard(game);
                pos = (Pos){cursX, 7-cursY};
                valid = possible(pos, game);
                displayMoves(valid);
                break;
            case 's':
                move.src = (Pos){cursX, 7-cursY};
                move.piece = value(move.src, game);
                break;
            case 'd':
                move.dst = (Pos){cursX, 7-cursY};
                move.capture = value(move.dst, game);
                ret = execMove(move, game);
                if(ret > 0) {
                    printBoard(game);
                }
                switch(ret) {
                    case TURN:
                        mvprintw(MSG, 0, "It's not your turn.");
                        break;
                    case INVALID:
                        mvprintw(MSG, 0, "That piece can't move like that.");
                        break;
                    case THREAT:
                        mvprintw(MSG, 0, "That would leave your king in check.");
                        break;
                    case CHECK:
                        mvprintw(MSG, 0, "Check!");
                        break;
                    case STALE:
                        mvprintw(MSG, 0, "Stalemate!");
                        break;
                    case MATE:
                        mvprintw(MSG, 0, "Checkmate!");
                        break;
                }
                clrtoeol;
                break;
        }
        mvchgat(1+cursY, 3*(1+cursX), 3, A_REVERSE, 3, NULL);
    }
}

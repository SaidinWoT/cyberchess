#ifndef _CHESS_H
#define _CHESS_H

#define REPS " PNK?BRQ!pnk/brq"

typedef int Row;

typedef enum _Type {EMPTY, PAWN, KNIGHT, KING, ENP, BISHOP, ROOK, QUEEN} Type;
typedef enum _Color {WHITE = 0x0, BLACK = 0x8} Color;

typedef struct _Pos {
    signed char \
        file:4, \
        rank:4;
} Pos;

typedef struct _Move {
    unsigned char \
        piece:4, \
        capture:4;
    Pos src;
    Pos dst;
    struct _Move *next;
} Move;

typedef struct _Game {
    struct {
        unsigned int \
            castle:4, \
            enp:3, \
            bcap:3, \
            wcap:3, \
            brow:1, \
            wrow:1, \
            color:1;
    } info;
    Pos king[2];
    Row capture[2][2];
    Row board[8];
} Game;

extern Game *newGame();
extern char value(Pos spot, Game *game);
extern Move *possible(Pos spot, Game *game);
extern void printBoard(Game *game);

#endif /* !_CHESS_H */

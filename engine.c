#include <ncurses.h>
#include <stdlib.h>
#include "chess.h"

#define COLOR(file,rank) (((file) + (rank)) % 2 ? 1 : 2)

char inline value(Pos spot, Game *game) {
    return (game->board[spot.rank] >> (spot.file << 2)) & 0xF;
}

void inline unset(Pos spot, Game *game) {
    game->board[spot.rank] &= ~(0xF << (spot.file << 2));
}

void inline set(Pos spot, char piece, Game *game) {
    game->board[spot.rank] |= (piece << (spot.file << 2));
}

void enp(Game *game) {
    Pos temp = (Pos){game->info.enp & 0x7, game->info.color ? 2 : 5};
    if(game->info.enp) {
        unset(temp, game);
    }
}

static char castle(Move move, Game *game) {
    return 0;
}

void fixCastle(Move move, Game *game) {
    static Pos rooks[4] = {(Pos){0,0}, (Pos){0,7}, (Pos){7,0}, (Pos){7,7}};
    char i;
    for(i = 0; i < 4; ++i) {
        if(move.src.file == rooks[i].file && move.src.rank == rooks[i].rank) {
            game->info.castle &= ~(0x1 << i);
        }
    }
}

static char empty(Move move, Game *game) {
    return 0;
}

static char pawn(Move move, Game *game) {
    Pos diff;
    Pos temp;
    if(move.capture) {
        diff.file = move.dst.file - move.src.file;
        diff.rank = (move.piece & 0x8 ? 1 : -1);
        if((diff.file*diff.file == 1) && (move.src.rank == move.dst.rank + diff.rank)) {
            if((move.capture & 0x7) == ENP) {
                temp = (Pos){move.dst.file, move.dst.rank + diff.rank};
                unset(temp, game);
            }
            return 1;
        }
    }
    diff.rank = move.dst.rank - move.src.rank;
    temp = (Pos){move.src.file, (move.piece & 0x8 ? 5 : 2)};
    if((move.src.rank == (move.piece & 0x8 ? 6 : 1)) && (diff.rank*diff.rank == 4) && !(value(temp, game))) {
        temp = (Pos){move.dst.file, move.dst.rank + (move.piece & 0x8 ? 1 : -1)};
        set(temp, (ENP | (game->info.color << 3)), game);
        enp(game);
        game->info.enp = move.dst.file | 0x8;
        return 1;
    }
    return (move.src.file == move.dst.file) && (diff.rank*diff.rank == 1);
}

static char knight(Move move, Game *game) {
    Pos diff = (Pos){move.dst.file - move.src.file, move.dst.rank - move.src.rank};
    return diff.file * diff.file + diff.rank * diff.rank == 5;
}

static char king(Move move, Game *game) {
    Pos diff = {move.dst.file - move.src.file, move.dst.rank - move.src.rank};
    if(diff.file*diff.file <= 1 && diff.rank*diff.rank <= 1) {
        game->king[move.piece & BLACK ? 1 : 0] = move.dst;
        game->info.castle &= ~(0x3 << (game->info.color << 1));
        return 1;
    }
    if(diff.file*diff.file == 4 && castle(move, game)) {
        return 1;
    }
    return 0;
}

static char bishop(Move move, Game *game) {
    Pos diff = {move.dst.file - move.src.file, move.dst.rank - move.src.rank};
    Pos vect = {diff.file > 0 ? -1 : 1, diff.rank > 0 ? -1 : 1};
    Pos temp;
    if(diff.file*diff.file != diff.rank*diff.rank) {
        return 0;
    }
    for(diff.file += vect.file, diff.rank += vect.rank; diff.file != 0; diff.file += vect.file, diff.rank += vect.rank) {
        temp = (Pos){move.src.file + diff.file, move.src.rank + diff.rank};
        if(value(temp, game)) {
            return 0;
        }
    }
    return 1;
}

static char rook(Move move, Game *game) {
    Pos diff = {move.dst.file - move.src.file, move.dst.rank - move.src.rank};
    Pos vect = {diff.file > 0 ? -1 : 1, diff.rank > 0 ? -1 : 1};
    Pos temp;
    if(!diff.file) {
        for(diff.rank += vect.rank; diff.rank != 0; diff.rank += vect.rank) {
            temp = (Pos){move.src.file, move.src.rank + diff.rank};
            if(value(temp, game)) {
                return 0;
            }
        }
        fixCastle(move, game);
        return 1;
    }
    else if(!diff.rank) {
        for(diff.file += vect.file; diff.file != 0; diff.file += vect.file) {
            temp = (Pos){move.src.file + diff.file, move.src.rank};
            if(value(temp, game)) {
                return 0;
            }
        }
        fixCastle(move, game);
        return 1;
    }
    return 0;
}

char queen(Move move, Game *game) {
    return (rook(move, game) || bishop(move, game));
}

char valid(Move move, Game *game) {
    static char (*moves[8])(Move move, Game *game) = {empty, pawn, knight, king, empty, bishop, rook, queen}; 
    if(move.capture && !((move.capture ^ move.piece) >> 3)) {
        return 0;
    }
    if(!(*moves[move.piece & 0x7])(move, game)) {
        return 0;
    }
    return 1;
}

void doMove(Move move, Game *game) {
    unset(move.dst, game);
    set(move.dst, value(move.src, game), game);
    unset(move.src, game);
}

char check(Game *game) {
    Pos spot;
    Move toKing[2];
    toKing[0].dst = game->king[0];
    toKing[1].dst = game->king[1];
    toKing[0].capture = WHITE | KING;
    toKing[1].capture = BLACK | KING;
    for(spot.rank = 0; spot.rank >= 0; ++spot.rank) {
        for(spot.file = 0; spot.file >= 0; ++spot.file) {
            if((toKing[0].piece = toKing[1].piece = value(spot, game))) {
                toKing[0].src = toKing[1].src = spot;
                if(valid(toKing[game->info.color], game)) {
                    return 1;
                }
                if(valid(toKing[!game->info.color], game)) {
                    mvprintw(10, 0, "CHECK!");
                }
            }
        }
    }
    return 0;
}

void copyGame(Game *new, Game *old) {
    int n;
    for(n = 0; n < 8; ++n) {
        new->board[n] = old->board[n];
    }
    new->info = old->info;
    new->king[0] = old->king[0];
    new->king[1] = old->king[1];
}

char execMove(Move move, Game *game) {
    Game *new = malloc(sizeof(Game));
    if((move.piece >> 3) ^ game->info.color) {
        return 0;
    }
    if(!valid(move, game)) {
        free(new);
        return 0;
    }
    copyGame(new, game);
    enp(new);
    doMove(move, new);
    if(check(new)) {
        free(new);
        return 0;
    }
    copyGame(game, new);
    game->info.color = !game->info.color;
    free(new);
    return 1;
}

Game *newGame() {
    Game *new = malloc(sizeof(Game));
    new->board[0] = 0x62537526;
    new->board[1] = 0x11111111;
    new->board[2] = 0x00000000;
    new->board[3] = 0x00000000;
    new->board[4] = 0x00000000;
    new->board[5] = 0x00000000;
    new->board[6] = 0x99999999;
    new->board[7] = 0xeadbfdae;
    new->info.castle = 0xF;
    new->info.enp = 0;
    new->info.color = 0;
    new->king[0] = (Pos){4,0};
    new->king[1] = (Pos){4,7};
    return new;
}

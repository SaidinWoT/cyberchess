#include <ncurses.h>
#include <stdlib.h>
#include "chess.h"
#include "engine.h"

#define COLOR(file,rank) (((file) + (rank)) % 2 ? 1 : 2)

char inline value(Pos spot, Game *game) { /* Get value of nybble at spot {{{ */
    return (game->board[spot.rank] >> (spot.file << 2)) & 0xF;
} /* }}} */

char inline capval(char color, char row, char spot, Game *game) { /* Get value of nybble in capture zone {{{ */
    return (game->capture[color][row] >> (spot << 2)) & 0xF;
} /* }}} */

void inline unset(Pos spot, Game *game) { /* Unset nybble at spot {{{ */
    game->board[spot.rank] &= ~(0xF << (spot.file << 2));
} /* }}} */

void inline set(Pos spot, char piece, Game *game) { /* Set nybble at spot to piece {{{ */
    game->board[spot.rank] |= (piece << (spot.file << 2));
} /* }}} */

void enp(Game *game) { /* Remove old en passant values {{{ */
    Pos temp = (Pos){game->info.enp, game->info.color ? 2 : 5}; //  Identify correct spot
    if((value(temp, game) & 0x7) == ENP) {  // If spot is En Passant...
        unset(temp, game);                  // ...unset it
    }
} /* }}} */

static char castle(Move move, Game *game) { /* Deal with castling. NOT COMPLETE. {{{ */
    static Pos spots[4] = {(Pos){2,0}, (Pos){6,0}, (Pos){2,7}, (Pos){6,7}};     // Possible destinations
    static Pos checks[4] = {(Pos){3,0}, (Pos){5,0}, (Pos){3,7}, (Pos){5,7}};    // Possible passthrough spots
    char i;
    if(threatened(game->king[game->info.color], game)) {    // Cannot castle if starting in check
        return 0;
    }
    for(i = 0; i < 4; ++i) {    // Iterate through spots list to find desired castling
        if(move.dst.file == spots[i].file && move.dst.rank == spots[i].rank) {
            if(threatened(spots[i], game)) {    // Cannot castle into check
                return 0;
            }
            break;
        }
    }
    if(threatened(checks[i], game)) {   // Cannot castle through check
        return 0;
    }
    return 0;
} /* }}} */

void fixCastle(Move move, Game *game) { /* Correct the castling flag nybble based on rook or king movement {{{ */
    char i;
    static Pos rooks[4] = {(Pos){0,0}, (Pos){7,0}, (Pos){0,7}, (Pos){7,7}}; // Possible rook locations
    if((move.piece & 0x7) == ROOK) {
        for(i = 0; i < 4; ++i) {    // Iterate through rook spots to identify which one moved
            if(move.src.file == rooks[i].file && move.src.rank == rooks[i].rank) {
                game->info.castle &= ~(0x1 << i);   // Remove respective entry from castling flag nybble
            }
        }
    } else if ((move.piece & 0x7) == KING) {
        game->info.castle &= ~(0x3 << (game->info.color << 1)); // Remove respective entry from castling flag nybble
    }
} /* }}} */

static char empty(Move move, Game *game) { /* Empty movement. For completeness. {{{ */
    return 0; // Can't move nothing anywhere. Always fail.
} /* }}} */

static char pawn(Move move, Game *game) { /* Pawn movement {{{ */
    Pos diff;
    Pos temp;
    if(move.capture) {  // Capturing is special
        diff.file = move.dst.file - move.src.file;
        diff.rank = (move.piece & 0x8 ? 1 : -1);    // Determine direction of movement based on color
        if((diff.file*diff.file == 1) && (move.src.rank == move.dst.rank + diff.rank)) {
            if((move.capture & 0x7) == ENP) {   // Deal with capturing En Passant spots properly
                temp = (Pos){move.dst.file, move.dst.rank + diff.rank}; // Identify relevant pawn...
                unset(temp, game);                                      // ...and unset it
            }
            return 1;
        }
    }
    if(move.src.file != move.dst.file) { // Make sure pawn is remaining in its file if not capturing
        return 0;
    }
    diff.rank = move.dst.rank - move.src.rank;
    /* Deal with possible initial move of two spaces */
    temp = (Pos){move.src.file, (move.piece & 0x8 ? 5 : 2)};
    if((move.src.rank == (move.piece & 0x8 ? 6 : 1)) && (diff.rank*diff.rank == 4) && !(value(temp, game))) {
        temp = (Pos){move.dst.file, move.dst.rank + (move.piece & 0x8 ? 1 : -1)};
        set(temp, (ENP | (game->info.color << 3)), game);   // Stick in en passant marker
        enp(game);                                          // Clean up old markers - done in this function due to next line
        game->info.enp = move.dst.file;                     // Record new en passant marker in info
        return 1;
    }
    return diff.rank*diff.rank == 1;
} /* }}} */

static char knight(Move move, Game *game) { /* Knight movement {{{ */
    Pos diff = (Pos){move.dst.file - move.src.file, move.dst.rank - move.src.rank};
    return diff.file * diff.file + diff.rank * diff.rank == 5;
} /* }}} */

static char king(Move move, Game *game) { /* King movement {{{ */
    Pos diff = (Pos){move.dst.file - move.src.file, move.dst.rank - move.src.rank};
    if(diff.file*diff.file <= 1 && diff.rank*diff.rank <= 1) {
        game->king[move.piece & BLACK ? 1 : 0] = move.dst;  // Update king location for check function
        return 1;
    }
    if(castle(move, game)) {    // Determine if castling
        return 1;
    }
    return 0;
} /* }}} */

static char bishop(Move move, Game *game) { /* Bishop movement {{{ */
    Pos diff = (Pos){move.dst.file - move.src.file, move.dst.rank - move.src.rank};
    Pos vect = {diff.file > 0 ? -1 : 1, diff.rank > 0 ? -1 : 1};
    Pos temp;
    if(diff.file*diff.file != diff.rank*diff.rank) {
        return 0; // Don't waste time if it's not a diagnoal move
    }
    for(diff.file += vect.file, diff.rank += vect.rank; diff.file != 0; diff.file += vect.file, diff.rank += vect.rank) {
        temp = (Pos){move.src.file + diff.file, move.src.rank + diff.rank};
        if(value(temp, game) & 0x3) {
            return 0;
        }
    }
    return 1;
} /* }}} */

static char rook(Move move, Game *game) { /* Rook movement {{{ */
    Pos diff = (Pos){move.dst.file - move.src.file, move.dst.rank - move.src.rank};
    Pos vect = (Pos){diff.file > 0 ? -1 : 1, diff.rank > 0 ? -1 : 1};
    Pos temp;
    if(!diff.file) { // Moving along a file?
        for(diff.rank += vect.rank; diff.rank != 0; diff.rank += vect.rank) {
            temp = (Pos){move.src.file, move.src.rank + diff.rank};
            if(value(temp, game) & 0x3) {
                return 0;
            }
        }
        return 1;
    } else if(!diff.rank) { // Moving along a rank?
        for(diff.file += vect.file; diff.file != 0; diff.file += vect.file) {
            temp = (Pos){move.src.file + diff.file, move.src.rank};
            if(value(temp, game) & 0x3) {
                return 0;
            }
        }
        return 1;
    }
    return 0;
} /* }}} */

char queen(Move move, Game *game) { /* Queen movement {{{ */
    return (rook(move, game) || bishop(move, game));    // Is it moving like a rook or bishop? Good.
} /* }}} */

char valid(Move move, Game *game) { /* Is the move valid? {{{ */
    static char (*moves[8])(Move move, Game *game) = {empty, pawn, knight, king, empty, bishop, rook, queen}; 
    if(move.capture && !((move.capture ^ move.piece) >> 3) && (move.capture & 0x7 != ENP)) {
        return 0;
    }
    if(!(*moves[move.piece & 0x7])(move, game)) {
        return 0;
    }
    return 1;
} /* }}} */

void doMove(Move move, Game *game) { /* Actually update the board. {{{ */
    unset(move.dst, game);
    set(move.dst, value(move.src, game), game);
    unset(move.src, game);
} /* }}} */

char threatened(Pos spot, Game *game) { /* Is spot threatened by any piece? {{{ */
    Pos iter;
    Move move;
    move.dst = spot;
    move.capture = value(spot, game);
    for(iter.rank = 0; iter.rank >= 0; ++iter.rank) {
        for(iter.file = 0; iter.file >= 0; ++iter.file) {
            move.src = iter;
            move.piece = value(iter, game);
            if(valid(move, game)) {
                return 1;
            }
        }
    }
    return 0;
} /* }}} */

char check(Game *game) { /* Is either king in check? NOTE: This logic may be moved to the execMove() function {{{ */
    if(threatened(game->king[game->info.color], game)) {
        return 1;
    }
    if(threatened(game->king[!game->info.color], game)) {
        return 0;
    }
    return 0;
} /* }}} */

/* Old logic below {{{ */
/*    Pos spot;
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
                }
            }
        }
    }
    return 0;
}*/
/* }}} */

void copyMove(Move *new, Move old) { /* Make pointer out of move object for possible() {{{ */
    new->piece = old.piece;
    new->capture = old.capture;
    new->src = old.src;
    new->dst = old.dst;
    new->next = old.next;
} /* }}} */

void copyGame(Game *new, Game *old) { /* Copy game from one pointer to another {{{ */
    int n;
    for(n = 0; n < 8; ++n) {
        new->board[n] = old->board[n];
    }
    new->info = old->info;
    new->king[0] = old->king[0];
    new->king[1] = old->king[1];
} /* }}} */

void capture(Move move, Game *game) { /* Properly execute a capture, relocating piece to capture zone {{{ */
    if((move.capture & 0x7) == ENP) { // Correcting for En Passant
        move.capture = 0;
        move.capture |= ((move.piece & 0x7) == PAWN) ? PAWN | (!game->info.color) << 3 : EMPTY;
    }
    if(move.capture) {
        game->capture[game->info.color][game->info.color ? game->info.brow : game->info.wrow] |= (move.capture << ((game->info.color ? game->info.bcap : game->info.wcap) << 2));
        if(game->info.color) {
            if(!++game->info.bcap) {
                ++game->info.brow;
            } 
        } else {
            if(!++game->info.wcap) {
                ++game->info.wrow;
            }
        }
    }
} /* }}} */

char execMove(Move move, Game *game) { /* Actually execute a move! Lots of logic in here. {{{ */
    Game *new = malloc(sizeof(Game));
    Game *save = malloc(sizeof(Game));
    copyGame(save, game);   // Hold on to the game state if we need to bail further down
    if((move.piece >> 3) ^ game->info.color) {  // Is the wrong color trying to move? Don't let them.
        free(save);
        free(new);
        return 0;
    }
    if(!valid(move, game)) {    // Is the move invalid? Don't let it happen.
        free(save);
        free(new);
        return 0;
    }
    copyGame(new, game);    // Make a copy of the game to test changes on
    enp(new);   // Unset en passant markers on the copy
    if(game->info.castle) { // Modify the castling flag nybble as needed
        fixCastle(move, new);
    }
    doMove(move, new);  // Actually do the move on the copy
    if(check(new)) {    // If this leaves own king in check, bail out
        copyGame(game, save);
        free(save);
        free(new);
        return 0;
    }
    copyGame(game, new);    // Commit to the change
    capture(move, game);    // Properly address captures
    game->info.color = !game->info.color;   // Change whose turn it is
    free(save);
    free(new);
    return 1;
} /* }}} */

Move *possible(Pos spot, Game *game) { /* Produce a list of valid moves. Currently terminates with a Move at 0,0 {{{ */
    Move *list = malloc(sizeof(Move));
    Move *curr = list;
    Move move;
    Pos iter;

    move.src = spot;
    move.piece = value(spot, game);

    for(iter.rank = 0; iter.rank >= 0; ++iter.rank) {   // Iterate through everything
        for(iter.file = 0; iter.file >= 0; ++iter.file) {
            move.dst = iter;
            move.capture = value(iter, game);
            if(valid(move, game)) { // If the move is valid, add it to the list
                /* This currently causes en passant markers to be placed if
                 * pawns would place them. This needs to be fixed. */
                copyMove(curr, move);
                curr->next = malloc(sizeof(Move));
                curr = curr->next;
            }
        }
    }
    return list;
} /* }}} */

Game *newGame() { /* Create a clean game {{{ */
    Game *new = malloc(sizeof(Game));
    new->board[0] = 0x62537526;
    new->board[1] = 0x11111111;
    new->board[2] = 0x00000000;
    new->board[3] = 0x00000000;
    new->board[4] = 0x00000000;
    new->board[5] = 0x00000000;
    new->board[6] = 0x99999999;
    new->board[7] = 0xeadbfdae;
    new->capture[0][0] = 0x00000000;
    new->capture[0][1] = 0x00000000;
    new->capture[1][0] = 0x00000000;
    new->capture[1][1] = 0x00000000;
    new->info.castle = 0xF;
    new->info.enp = 0;
    new->info.wrow = 0;
    new->info.brow = 0;
    new->info.wcap = 0;
    new->info.bcap = 0;
    new->info.color = 0;
    new->king[0] = (Pos){4,0};
    new->king[1] = (Pos){4,7};
    return new;
} /* }}} */

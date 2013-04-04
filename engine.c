#include <stdlib.h>
#include "chess.h"
#include "engine.h"

#define color(val) (val >> 3)

Game *newGame(char (*getfunc)()) { /* Create a clean game {{{1 */
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
    new->info.wrow = 0;
    new->info.brow = 0;
    new->info.wcap = 0;
    new->info.bcap = 0;
    new->info.color = 0;
    new->king[0] = (Pos){4,0};
    new->king[1] = (Pos){4,7};
    new->fp = getfunc;
    return new;
}

void copyGame(Game *new, Game *game) { /* Copy the values of one game pointer to another {{{1 */
    int n;
    for(n = 0; n < 8; ++n) {
        new->board[n] = game->board[n];
    }
    new->capture[0][0] = game->capture[0][0];
    new->capture[0][1] = game->capture[0][1];
    new->capture[1][0] = game->capture[1][0];
    new->capture[1][1] = game->capture[1][1];
    new->info = game->info;
    new->king[0] = game->king[0];
    new->king[1] = game->king[1];
}

char inline value(Pos spot, Game *game) { /* Get value of nybble at spot {{{1 */
    return (game->board[spot.rank] >> (spot.file << 2)) & 0xF;
}

char inline capval(char color, char row, char spot, Game *game) { /* Get value of nybble in capture zone {{{1 */
    return (game->capture[color][row] >> (spot << 2)) & 0xF;
}

void inline unset(Pos spot, Game *game) { /* Unset nybble at spot {{{1 */
    game->board[spot.rank] &= ~(0xF << (spot.file << 2));
}

void inline set(Pos spot, char piece, Game *game) { /* Set nybble at spot to piece {{{1 */
    game->board[spot.rank] |= (piece << (spot.file << 2));
}

Pos inline movediff(Move move) { /* Return the distances travelled in a Pos {{{1 */
    return (Pos){move.dst.file - move.src.file, move.dst.rank - move.src.rank};
}

static void enp(Game *game) { /* Clean up any old en passant markers {{{1 */
    Pos spot = (Pos){7, game->info.color ? 2 : 5};  // Set appropriate rank
    for(; spot.file >= 0; --spot.file) {            // Iterate through spots in rank
        if((value(spot, game) & 0x7) == ENP) {
            unset(spot, game);                      // Unset if en passant marker
        }
    }
}

char threatened(char color, Pos spot, Game *game) { /* Check if spot is threatened, assuming it matches color {{{1 */
    Pos iter;
    Move move;
    move.dst = spot;
    move.capture = value(spot, game);
    for(iter.rank = 7; iter.rank >= 0; --iter.rank) {
        for(iter.file = 7; iter.file >= 0; --iter.file) {   // Iterate through spots on board
            move.src = iter;
            move.piece = value(iter, game);
            if((color != color(move.piece)) && valid(move, game)) {
                return 1;   // Return 1 if opposing piece can legally move to spot
            }
        }
    }
    return 0;   // Return 0 if no pieces can legally move to spot
}

static void fixCastle(Move move, Game *game) { /* Fix up the castling flag nybble as needed {{{1 */
    static Pos rooks[2][2] = { {(Pos){0, 0}, (Pos){7, 0} }, {(Pos){0, 7}, (Pos){7, 7} } };
    char c = color(move.piece);
    if((move.piece & 0x7) == ROOK) {    // If rook, unset bit based on src location
        if((move.src.file == rooks[c][0].file) && (move.src.rank == rooks[c][0].rank)) {
            game->info.castle &= ~(0x1 << (2*c));
        }
        if((move.src.file == rooks[c][1].file) && (move.src.rank == rooks[c][1].rank)) {
            game->info.castle &= ~(0x1 << (2*c+1));
        }
    }
    if((move.piece & 0x7) == KING) {    // If king, unset bits based on color
        game->info.castle &= ~(0x3 << 2*c);
    }
}

static char castle(Move move, Game *game) { /* Deal with castling {{{1 */
    static Pos rooks[2][2] = { {(Pos){0, 0}, (Pos){7, 0} }, {(Pos){0, 7}, (Pos){7, 7} } }; // Rook starting positions
    static Pos empty[2] = {(Pos){1, 0}, (Pos){1, 7}};                                   // Empty squares for Q-side
    static Pos kdst[2][2] = { {(Pos){2, 0}, (Pos){6, 0} }, {(Pos){2, 7}, (Pos){6, 7} } };  // Destinations for kings
    static Pos rdst[2][2] = { {(Pos){3, 0}, (Pos){5, 0} }, {(Pos){3, 7}, (Pos){5, 7} } };  // Destinations for rooks
    char i;
    char c = color(move.piece);
    if((move.dst.file == kdst[c][0].file) && move.dst.rank == kdst[c][0].rank) {
        if(value(kdst[c][0], game) || value(rdst[c][0], game) || value(empty[c], game)) {
            return 0;   // Fail if spots aren't empty
        }
        i = 0;
    } else if((move.dst.file == kdst[c][1].file) && move.dst.rank == kdst[c][1].rank) {
        if(value(kdst[c][1], game) || value(rdst[c][1], game)) {
            return 0;   // Fail if spots aren't empty
        }
        i = 1;
    } else {
        return 0;   // Fail if king is moving somewhere that isn't a castling destination
    }
    if(!((0x1 << (2*c+i)) & game->info.castle)) {
        return 0;   // Fail if king or rook have moved
    }
    if(threatened(c, game->king[c], game) || threatened(c, kdst[c][i], game) || threatened(c, rdst[c][i], game)) {
        return 0;   // Fail if starting in, passing through, or ending in check
    }
    Move rook = (Move){ROOK, EMPTY, rooks[c][i], rdst[c][i]};
    doMove(rook, game); // Move rook to its destination
    return 1;   // Send success so king will move
}

static char mate(char color, Game *game) { /* Check for {check,stale}mate {{{1 */
    Move *moves;
    Pos iter;
    for(iter.rank = 7; iter.rank >= 0; --iter.rank) {
        for(iter.file = 7; iter.file >= 0; --iter.file) {
            if(color(value(iter, game)) == color) { // Make sure we're only looking at threatened color
                moves = possible(iter, game);
                if(moves->next) {   // If moves are available, moves->next will be non-NULL
                    return 0;   // If anyone can move, it's not mate
                }
            }
        }
    }
    return 1; // If no one could move, it was mate
}

static Move promote(Move move, Game *game) { /* Promote a pawn. Calls game's callback function to determine piece {{{1 */
    if((move.piece & 0x7) != PAWN) {
        return; // You can't promote a non-pawn
    }
    if(move.dst.rank != (color(move.piece) ? 0 : 7)) {
        return; // Pawns only promote at the end
    }
    unset(move.src, game);
    set(move.src, (move.piece & 0x8) | (game->fp() & 0x7), game);   // Use callback to determine new value
}

static char empty(Move move, Game *game) { /* Empty movement. For completeness. {{{1 */
    return 0;
}

static char pawn(Move move, Game *game) { /* Pawn movement {{{1 */
    Pos diff = movediff(move);
    Pos temp;
    char dir = (color(move.piece) ? -1 : 1);    // Proper direction of motion based on color
    if(move.capture) {  // Pawn captures need special logic
        if((diff.file*diff.file == 1) && (move.src.rank + dir == move.dst.rank)) {
            if((move.capture & 0x7) == ENP) {
                temp = (Pos){move.dst.file, move.src.rank};
                unset(temp, game);  // Unset the pawn that matches the en passant marker
            }
            return 1;
        }
        return 0;
    }
    if(move.src.file != move.dst.file) {
        return 0;   // Fail if the pawn is trying to leave its file without a capture
    }
    temp = (Pos){move.src.file, (color(move.piece) ? 5 : 2)};
    if((move.src.rank == (color(move.piece) ? 6 : 1)) && (diff.rank*diff.rank == 4) && !(value(temp, game))) {
        set(temp, (ENP | (game->info.color << 3)), game);   // Leave an en passant marker
        return 1;
    }
    return diff.rank == dir;
}

static char knight(Move move, Game *game) { /* Knight movement {{{1 */
    Pos diff = movediff(move);
    return diff.file*diff.file + diff.rank*diff.rank == 5;
}

static char king(Move move, Game *game) { /* King movement {{{1 */
    Pos diff = movediff(move);
    if(diff.file*diff.file <= 1 && diff.rank*diff.rank <= 1 || castle(move, game)) {
        game->king[color(move.piece)] = move.dst;   // Record new king location
        return 1;
    }
    return 0;
}

static char bishop(Move move, Game *game) { /* Bishop movement {{{1 */
    Pos diff = movediff(move);
    Pos vect = {diff.file > 0 ? -1 : 1, diff.rank > 0 ? -1 : 1};
    Pos temp;
    if(diff.file*diff.file != diff.rank*diff.rank) {
        return 0; // Don't waste time if it's not a diagnoal move
    }
    for(diff.file += vect.file, diff.rank += vect.rank; diff.file != 0; diff.file += vect.file, diff.rank += vect.rank) {
        temp = (Pos){move.src.file + diff.file, move.src.rank + diff.rank};
        if(value(temp, game) & 0x3) {
            return 0; // Fail if spot is not empty or en passant marker
        }
    }
    return 1;
}

static char rook(Move move, Game *game) { /* Rook movement {{{1 */
    Pos diff = movediff(move);
    Pos vect = (Pos){diff.file > 0 ? -1 : 1, diff.rank > 0 ? -1 : 1};
    Pos temp;
    if(!diff.file) { // Moving along a file?
        for(diff.rank += vect.rank; diff.rank != 0; diff.rank += vect.rank) {
            temp = (Pos){move.src.file, move.src.rank + diff.rank};
            if(value(temp, game) & 0x3) {
                return 0; // Fail if spot is not empty or en passant marker
            }
        }
        return 1;
    } else if(!diff.rank) { // Moving along a rank?
        for(diff.file += vect.file; diff.file != 0; diff.file += vect.file) {
            temp = (Pos){move.src.file + diff.file, move.src.rank};
            if(value(temp, game) & 0x3) {
                return 0; // Fail if spot is not empty or en passant marker
            }
        }
        return 1;
    }
    return 0; // Fail if not moving horizontally or vertically
}

static char queen(Move move, Game *game) { /* Queen movement {{{1 */
    return (rook(move, game) || bishop(move, game));    // Queen moves either like a rook or bishop
}

char valid(Move move, Game *game) { /* Is the move valid? {{{1 */
    static char (*moves[8])(Move move, Game *game) = {empty, pawn, knight, king, empty, bishop, rook, queen};
    Pos diff = movediff(move);
    if(diff.rank == 0 & diff.file == 0) {
        return 0;   // Fail if trying to move to self
    }
    if(move.capture && (color(move.capture) == color(move.piece))) {
        if(move.capture & 0x3) {
            return 0;   // Fail if trying to capture own piece
        } else {
            move.capture = EMPTY;   // Kill rogue en passant markers (none should exist of own color)
        }
    }
    return (*moves[move.piece & 0x7])(move, game);  // Rest of the work done by helper functions
}

void doMove(Move move, Game *game) { /* Actually update the board. {{{1 */
    unset(move.dst, game);
    set(move.dst, value(move.src, game), game);
    unset(move.src, game);
}

void capture(Move move, Game *game) { /* Properly execute a capture, relocating piece to capture zone {{{1 */
    if((move.capture & 0x7) == ENP) {   // Make en passant markers show up as pawns in capture zone
        move.capture = ((move.piece & 0x7) == PAWN) ? PAWN | (!game->info.color) << 3 : EMPTY;
    }
    if(move.capture) {
        game->capture[game->info.color][game->info.color ? game->info.brow : game->info.wrow] |= (move.capture << ((game->info.color ? game->info.bcap : game->info.wcap) << 2));
        if(game->info.color) {
            if(!++game->info.bcap) {    // Increment black capture zone spot
                ++game->info.brow;      // Move on to next row as needed
            }
        } else {
            if(!++game->info.wcap) {    // Increment white capture zone spot
                ++game->info.wrow;      // Move on to next row as needed
            }
        }
    }
}

char execMove(Move move, Game *game) { /* Actually execute a move! Lots of logic in here. {{{1 */
    Game *save = malloc(sizeof(Game));
    copyGame(save, game);   // Hold on to current board state if we need to bail
    if(color(move.piece) ^ game->info.color) {
        free(save);
        return TURN;    // Fail if wrong color is trying to move
    }
    if(!valid(move, game)) {
        free(save);
        return INVALID; // Fail if move is invalid
    }
    enp(game);  // Remove old en passant markers
    promote(move, game);    // See if we're promoting a pawn
    doMove(move, game);     // Execute the move
    if(threatened(game->info.color, game->king[game->info.color], game)) {
        copyGame(game, save);
        free(save);
        return THREAT;  // Fail if own king will be threatened
    }
    if(!game->info.castle) {
        fixCastle(move, game);  // Unset castling flags as needed
    }
    capture(move, game);    // Stick captured pieces in the capture zone
    game->info.color = !game->info.color;   // Switch whose turn it is
    game->info.check = threatened(game->info.color, game->king[game->info.color], game);    // See if move caused check
    game->info.mate = mate(game->info.color, game); // See if opponent is capable of moving
    if(game->info.check & game->info.mate) {
        return MATE;    // Return checkmate if opponent is in check and cannot move
    }
    if(game->info.mate) {
        return STALE;   // Return stalemate if opponent cannot move but is not in check
    }
    if(game->info.check) {
        return CHECK;   // Return check if opponent is in check and can move
    }
    free(save);
    return 1;   // Return 1 if nothing is special
}

Move *possible(Pos spot, Game *game) { /* Produce a list of valid moves. Terminates with one extra Move node {{{1 */
    Game *test = malloc(sizeof(Game));
    Move *list = malloc(sizeof(Move));
    Move *curr = list;
    Move move;
    Pos iter;

    list->next = NULL;
    move.src = spot;
    move.piece = value(spot, game);
    if(color(move.piece) != game->info.color) {
        return list;    // No reason to give valid moves for pieces that cannot move right now
    }

    for(iter.rank = 7; iter.rank >= 0; --iter.rank) {
        for(iter.file = 7; iter.file >= 0; --iter.file) {
            copyGame(test, game);   // Work in a disposable environment
            move.dst = iter;
            move.capture = value(iter, test);
            if(valid(move, test)) {
                doMove(move, test); // Actually make the move (in our disposable environment)
                if(!threatened(test->info.color, test->king[test->info.color], test)) {
                    *curr = move;   // If it's valid, stick it on the list
                    curr->next = malloc(sizeof(Move));
                    curr = curr->next;
                }
            }
        }
    }

    free(test);

    return list;
}

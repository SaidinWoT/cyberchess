#ifndef _ENGINE_H
#define _ENGINE_H
char inline value(Pos spot, Game *game);
char inline capval(char color, char row, char spot, Game *game);
void inline unset(Pos spot, Game *game);
void inline set(Pos spot, char piece, Game *game);
void enp(Game *game);
static char castle(Move move, Game *game);
void fixCastle(Move move, Game *game);
static char empty(Move move, Game *game);
static char pawn(Move move, Game *game);
static char knight(Move move, Game *game);
static char king(Move move, Game *game);
static char bishop(Move move, Game *game);
static char rook(Move move, Game *game);
char queen(Move move, Game *game);
char valid(Move move, Game *game);
void doMove(Move move, Game *game);
char threatened(Pos spot, Game *game);
char check(Game *game);
void copyMove(Move *new, Move old);
void copyGame(Game *new, Game *old);
void capture(Move move, Game *game);
char execMove(Move move, Game *game);
Move *possible(Pos spot, Game *game);
Game *newGame();
#endif /* !_ENGINE_H */

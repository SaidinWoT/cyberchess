#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "chess.h"

typedef enum _Command { 
    ALPHA = 0x8, BRAVO, CHARLIE, DELTA, ECHO, FOXTROT, GOLF, HOTEL,           //8-15
        ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE,     //16-25
        RESIGN, DRAW, SAVE, LOAD, END, GAME,                            //26-31
        YES, NO,                                                        //32-33
        CASTLE, SIDE,                                                   //34-35
        SECONDS, MINUTES, HOUR, GLASS, INDIVIDUAL, PLAYER, ENTIRE,      //36-42
        TAKES, CAPTURE,                                                 //43-44
        MOVE, POSSIBLE,                                                 //45-46
        TERM} Command;                                                  //47

void parseCmd(const char *command, char *cmd);
char askUser(Move move);
void mcuInit(int fd);
void ardOut(int fd, char val);
void mcuMove(int fd, Move move, Game *game);
void mcuPos(int fd, Move *move);
void reqRep();
char *getInput();

int main(int argc, char **argv) {
    int fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_SYNC);
    int i;
    Game *game = newGame(askUser);
    char *command;
    char cmd[8];
    Move move;
    if(fd < 0) {
        free(game);
        return 1;
    }
    mcuInit(fd);
    while(!game->info.mate) {
        command = getInput();
        parseCmd(command, cmd);
        for(i = 0; cmd[i]; ++i) {
            printf("%d ", cmd[i]);
        }
        printf("\n");
        switch(cmd[0]) {
            case MOVE:
                move.piece = (game->info.color << 3) | cmd[1];
                move.src.file = cmd[2] - ALPHA;
                move.src.rank = cmd[3] - ONE;
                move.dst.file = cmd[5] - ALPHA;
                move.dst.rank = cmd[6] - ONE;
                move.capture = value(move.dst, game);
                if(execMove(move, game) > 0) {
                    mcuMove(fd, move, game);
                } else {
                    printf("Fail move.\n");
                }
                break;
            case POSSIBLE:
                move.src.file = cmd[2] - ALPHA;
                move.src.rank = cmd[3] - ONE;
                mcuPos(fd, possible(move.src, game));
                break;
            default:
                reqRep();
        }
    }
    free(game);
    return 0;
}

void reqRep() {
    printf("Sorry, didn't understand your command. Please repeat it.\n");
}

char *getInput() {
    char *str = malloc(BUFSIZ*sizeof(char));
    printf("Please type things.\n");
    fgets(str, BUFSIZ-1, stdin);
    return str;
}

char askUser(Move move) {
    return QUEEN;
}

Move getMove() {
    Move move;
    return move;
}

void ardOut(int fd, char val) {
    write(fd, &val, 1);
    printf("%d\n", val);
}

void mcuInit(int fd) {
    ardOut(fd, 'h');
    ardOut(fd, 30);
    ardOut(fd, 25);
    ardOut(fd, 'e');
}

void mcuMove(int fd, Move move, Game *game) {
    ardOut(fd, 'm');
    if(move.capture) {
        ardOut(fd, move.dst.file + 2);
        ardOut(fd, move.dst.rank);
        if(move.capture & 0x8) {
            if(game->info.brow && !game->info.bcap) {
                ardOut(fd, 0);
                ardOut(fd, 7);
            } else {
                ardOut(fd, game->info.brow);
                ardOut(fd, game->info.bcap - 1);
            }
        } else {
            if(game->info.wrow && !game->info.wcap) {
                ardOut(fd, 11);
                ardOut(fd, 7);
            } else {
                ardOut(fd, game->info.wrow + 10);
                ardOut(fd, game->info.wcap - 1);
            }
        }
    }
    ardOut(fd, move.src.file + 2);
    ardOut(fd, move.src.rank);
    ardOut(fd, move.dst.file + 2);
    ardOut(fd, move.dst.rank);
    ardOut(fd, 'x');
}

void mcuPos(int fd, Move *move) {
    Move *curr;
    curr = move;
    ardOut(fd, 'p');
    for(; curr->next; curr = curr->next) {
        ardOut(fd, curr->dst.file + 2);
        ardOut(fd, curr->dst.rank);
        ardOut(fd, 2);
    }
    ardOut(fd, 'x');
}

void parseCmd(const char *command, char *cmd) {
    char i = -1;
    while(1) {
        switch(command[0]) {
            case '\0':
                return;
            case 'B': switch(command[1]) {
                          case 'I': command += 7; cmd[++i] = BISHOP; break;
                          case 'R': command += 6; cmd[++i] = BRAVO; break;
                      } break;
            case 'C': switch(command[2]) {
                          case 'P': command += 8; cmd[++i] = CAPTURE; break;
                          case 'S': command += 7; cmd[++i] = CASTLE; break;
                          case 'A': command += 8; cmd[++i] = CHARLIE; break;
                      } break;
            case 'D': switch(command[1]) {
                          case 'E': command += 6; cmd[++i] = DELTA; break;
                          case 'R': command += 5; cmd[++i] = DRAW; break;
                      } break;
            case 'E': switch(command[2]) {
                          case 'H': command += 5; cmd[++i] = ECHO; break;
                          case 'G': command += 6; cmd[++i] = EIGHT; break;
                          case 'D': command += 4; cmd[++i] = END; break;
                          case 'T': command += 7; cmd[++i] = ENTIRE; break;
                      } break;
            case 'F': switch(command[2]) {
                          case 'V': command += 5; cmd[++i] = FIVE; break;
                          case 'U': command += 5; cmd[++i] = FOUR; break;
                          case 'X': command += 8; cmd[++i] = FOXTROT; break;
                      } break;
            case 'G': switch(command[1]) {
                          case 'A': command += 5; cmd[++i] = GAME; break;
                          case 'L': command += 6; cmd[++i] = GLASS; break;
                          case 'O': command += 5; cmd[++i] = GOLF; break;
                      } break;
            case 'H': switch(command[2]) {
                          case 'T': command += 6; cmd[++i] = HOTEL; break;
                          case 'U': command += 5; cmd[++i] = HOUR; break;
                      } break;
            case 'M': switch(command[1]) {
                          case 'I': command += 8; cmd[++i] = MINUTES; break;
                          case 'O': command += 5; cmd[++i] = MOVE; break;
                      } break;
            case 'N': switch(command[2]) {
                          case 'G': command += 6; cmd[++i] = KNIGHT; break;
                          case 'N': command += 5; cmd[++i] = NINE; break;
                          case '\0': command += 3; cmd[++i] = NO; break;
                      } break;
            case 'P': switch(command[1]) {
                          case 'A': command += 5; cmd[++i] = PAWN; break;
                          case 'L': command += 7; cmd[++i] = PLAYER; break;
                          case 'O': command += 9; cmd[++i] = POSSIBLE; break;
                      } break;
            case 'R': switch(command[1]) {
                          case 'E': command += 7; cmd[++i] = RESIGN; break;
                          case 'O': command += 5; cmd[++i] = ROOK; break;
                      } break;
            case 'S': switch(command[2]) {
                          case 'V': switch(command[1]) {
                                        case 'A': command += 5; cmd[++i] = SAVE; break;
                                        case 'E': command += 6; cmd[++i] = SEVEN; break;
                                    } break;
                          case 'C': command += 8; cmd[++i] = SECONDS; break;
                          case 'D': command += 5; cmd[++i] = SIDE; break;
                          case 'X': command += 4; cmd[++i] = SIX; break;
                      } break;
            case 'T': switch(command[1]) {
                          case 'A': command += 6; cmd[++i] = TAKES; break;
                          case 'H': command += 6; cmd[++i] = THREE; break;
                          case 'W': command += 4; cmd[++i] = TWO; break;
                      } break;
            case 'A': command += 6; cmd[++i] = ALPHA; break;
            case 'I': command += 11; cmd[++i] = INDIVIDUAL; break;
            case 'K': command += 5; cmd[++i] = KING; break;
            case 'L': command += 5; cmd[++i] = LOAD; break;
            case 'O': command += 4; cmd[++i] = ONE; break;
            case 'Q': command += 6; cmd[++i] = QUEEN; break;
            case 'Y': command += 4; cmd[++i] = YES; break;
            case 'Z': command += 5; cmd[++i] = ZERO; break;
        }
    }
}


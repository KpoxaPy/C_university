#ifndef _GAME_H_
#define _GAME_H_

#include "main.h"
#include "../stuff/buffer.h"

#define GAME_DFL_PRODUCT_STUFF 1
#define GAME_DFL_PRODUCT_PRICE 2000

#define GAME_DFL_MATERIAL_RENT_PRICE 300
#define GAME_DFL_PRODUCT_RENT_PRICE 500
#define GAME_DFL_FACTORY_RENT_PRICE 1000

#define GAME_DFL_FACTORY_BUILD_PRICE 5000
#define GAME_DFL_FACTORY_BUILD_TIME 5

#define GAME_DFL_FACTORIES 2
#define GAME_DFL_MATERIAL 4
#define GAME_DFL_PRODUCT 2
#define GAME_DFL_MONEY 500

#define GAME_STATE_1 0
#define GAME_STATE_2 1
#define GAME_STATE_3 2
#define GAME_STATE_4 3
#define GAME_STATE_5 4
#define GAME_STATE_DFL GAME_STATE_3

#define BID_NONE 0
#define BID_MAT 1
#define BID_PROD 2
#define BID_PLPROD 3
#define BID_PLFAC 4

#define BID_SORTING_ASC 1
#define BID_SORTING_DESC 2

typedef struct game {
	int num;

	int gid;
	int nPlayers;

	int started;
	int turn;

	struct state {
		int s;
		int sellC;
		int sellMinP;
		int buyC;
		int buyMaxP;
	} state;
	int rMatPrice;
	int rProdPrice;
	int rFacPrice;
	int bFacPrice;
	int bFacTime;
	int prodPrice;
	int prodStuff;

	struct managedPlayer * players;
} Game;

typedef struct managedGame {
	struct game * game;
	struct managedGame * next;
} mGame;

typedef struct factory {
	int age;
	struct factory * next;
} Factory;

typedef struct player {
	int fd;
	int pid;
	char * nick;
	char * dummynick;
	int adm;
	int turned;

	int mat;
	int prod;
	int bf;
	Factory * cf;
	int money;

	struct sellBid {
		int c;
		int p;
	} sellBid;

	struct buyBid {
		int c;
		int p;
	} buyBid;

	struct prodBid {
		int c;
	} prodBid;

	Buffer * buf;
	struct game * game;
} Player;

typedef struct managedPlayer {
	struct player * player;
	struct managedPlayer * next;
} mPlayer;

typedef struct bid {
	int type;
	Player * p;
	int count;
	int price;
	struct bid * next;
} Bid;

typedef struct bidlist {
	Bid * f, * l;
	int sorting;
} BidList;

typedef struct turn {
	int turn;
	int state;
	BidList * prod;
	BidList * fac;
	BidList * sell;
	BidList * buy;
} Turn;

Game * newGame(void);
void remGame(Game *);

void setState(Game *, int);
void changeState(Game *);

Factory * newFactory();
void remFactory(Factory *);
void addFactory(Player *);
void clearFactories(Player *);
int checkFactories(Player *);
int countFactories(Player *);

Player * newPlayer(int);
void remPlayer(Player *);
char * getNickname(Player *);

Bid * newBid(void);
void remBid(Bid *);
BidList * newBidList(int);
void remBidList(BidList *);
void delBidFromList(Bid *, BidList *);
void addBidToList(Bid *, BidList *);
int countBids(BidList *);

Turn * newTurn(void);
void remTurn(Turn *);

void fetchData(Player *);
void checkPlayerOnCommand(Player *);

void startGame(Game *);
Turn * turnGame(Game *);

#endif

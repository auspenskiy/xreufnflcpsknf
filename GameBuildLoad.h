#pragma once

#include <fstream>
#include <string>
#include "GameBuilder.h"
#include "game.h"

/*
Class GameBuildLoad: implementation of the build pattern used to load games from file
*/
class GameBuildLoad: public GameBuilder{
public:
  GameBuildLoad(int saveSlotNum);
  void buildMap();
  void buildPlayers();
  void buildGameState();
  
  Game * getResult();
  
private:
  int gameSaveSlotNum;
  int playerTurns;
  int playerIndex;
  int bonusArmies;
  
  Player ** playerArray;
  int numOfPlayers;
  Map * newMap;
  int numOfArmiesExchange;
  
  std::ifstream inStream;

};
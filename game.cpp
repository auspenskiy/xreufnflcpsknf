#include "game.h"
#include <sstream>
#include <iostream>
#include <cmath>
#include "GameBuilderDirector.h"
#include "GameBuildSave.h"

Game::Game(int newNumOfPlayers, Player ** playaArray, Map * newMap, int newPlayerTurns, int newPlayerIndex, int newBonusArmies, bool newJustLoaded){
	numOfPlayers = newNumOfPlayers;
	playerTurns = newPlayerTurns;
	playerIndex = newPlayerIndex;
	bonusArmies = newBonusArmies;
	justLoaded = newJustLoaded;
	
	playerArray = playaArray;
	
	totalBattles = 0;
	playersAlive = 0;
	
	for(int x = 0; x < newNumOfPlayers; x++){
	    totalBattles += playerArray[x]->getBattlesLost();
	    playersAlive += (int)playerArray[x]->getIsAlive();
	}
	
	map = newMap;
	
	if(!justLoaded){
	  map->setupCountryOwners(numOfPlayers);
	}
	map->setPlayerArrayInMap(playerArray);
	textview = new TextView(*map);
	dice = new Dice();

}

Game::~Game(){
	delete map;
	delete textview;
	delete dice;
	delete[] playerNames;
	delete[] playerArray;
}

int Game::play(){
	int choice;
	
	//Main Game Loop
	
	do{
		currentPlayer = playerArray[playerIndex];
		  map->notify();

		if (currentPlayer->getIsAlive()){

			View::inform("Round " + intToString(playerTurns / numOfPlayers + 1) + " : " + currentPlayer->getName() + " (player " + intToString(playerIndex) + ")'s turn");

			//reinforce, attack and move are the 3 actions a given player can do during his turn
			reinforce(playerIndex);
			
			do{
				View::inform("What would you like to do?");
				View::inform("1 - Attack");
				View::inform("2 - Fortify");
				View::inform("3 - Display statistics");
				View::inform("4 - Edit map");
				View::inform("5 - Save game");
				View::inform("6 - Quit");
				View::inform("0 - End turn");

				choice = View::getInt();
				switch (choice){
				case 0:
					break;
				case 1:
					attack(playerIndex);
					break;
				case 2:
					fortify(playerIndex);
					break;
				case 3:
					displayStatistics();
					break;
				case 4:
					//the map editor is simply a text editor
					system("Start notepad \"../World.map\"");
					break;
				case 5:
					saveGame();
					break;
				case 6:
					return 0;
					break;

				default:
					View::inform("Invalid input");
				}
			} while (choice != 0);

			if (currentPlayer->getNumCountriesOwned() == 0){
				currentPlayer->setDeath();
			}
		}
		else{
			continue;
		}
		playerTurns++;
		playerIndex = playerTurns % numOfPlayers;


		playersAlive = countPlayersAlive();

	} while (playersAlive > 1);

	//if a player wins, find that player
	Player* winner = findWinner();
	View::inform("The winner is " + winner->getName());


	return 0;
}


int Game::countPlayersAlive(){
	int alive = 0;
	for (int x = 0; x < numOfPlayers; x++){
		if (playerArray[x]->getIsAlive()){
			alive++;
		}
	}
	return alive;
}

Player* Game::findWinner(){
	Player *p = NULL;
	for (int x = 0; x < numOfPlayers; x++){
		if (playerArray[x]->getIsAlive()){
			p = playerArray[x];
		}
	}
	return p;
}

bool Game::countryExistsAndFriendly(std::string country, int playerIndex){
	if (!map->countryExists(country)){
		View::inform(country + " does not exist.");
		return false;
	}
	else if (map->getCountryOwnerIndex(country) != playerIndex){
		View::inform(country + " does not belong to you.");
		return false;
	}
	return true;
}

void Game::outputCountryList(std::list<std::string> countryList){
	//output list of countries
	std::list<std::string>::iterator iter = countryList.begin();
	while (iter != countryList.end()){
		View::inform("  " + *iter);
		iter++;
	}
}

void Game::fortify(int playerIndex){
	std::string answer;

	std::string sourceCountry;
	std::string destinationCountry;
	std::list<std::string> lst;
	int numToMove;

	//input loop: prompt user for a friendly country until they pick one with friendly neighbours
	do{
		//input loop: prompt user for a friendly country until they pick one with enough armies to move elsewhere
		do{
			View::prompt("Choose a country to move troops from.");
			sourceCountry = View::getString();
			//if country doesn't have enough armies to move some to a neighbour
			if (map->countryExists(sourceCountry) && map->getCountryOwnerIndex(destinationCountry) == playerIndex && map->getCountryArmies(sourceCountry) < 2){
				View::inform(sourceCountry + " does not have enough armies to move troops to another country");
			}

		} while (!(countryExistsAndFriendly(sourceCountry, playerIndex) && map->getCountryArmies(sourceCountry) > 2));

		//get list of all countries connected to the selected country
		lst = map->getConnectedFriendlyCountries(sourceCountry, playerIndex);

		//if the country has no countries connected to it (other than itself)
		if (lst.size() <= 1){
			View::inform(sourceCountry + " is not connected to any friendly neighbours to which it can move armies.");
		}
	} while (lst.size() <= 1);

	View::inform(sourceCountry + " is connected to:");
	outputCountryList(lst);

	//Input loop: prompt for a friendly country until they enter one on the list of connected countries
	do{
		View::prompt("Choose the country to fortify.");
		destinationCountry = View::getString();

		//if the entered country is not on the list of connected countries
		if (map->countryExists(destinationCountry) && map->getCountryOwnerIndex(destinationCountry) == playerIndex && !listContains(lst, destinationCountry)){
			View::inform(destinationCountry + " is not connected to " + sourceCountry + " and therefore cannot be fortified with armies from " + sourceCountry + ".");
		}
	} while (!(countryExistsAndFriendly(destinationCountry, playerIndex) && listContains(lst, destinationCountry)));

	//Input loop: prompt for the number of armies to move until they pick a number between 1 and the max they can move
	View::prompt(sourceCountry + " can move up to " + intToString(map->getCountryArmies(sourceCountry) - 1) + " armies to " + destinationCountry + ".  How many would you like to move?");
	do{
		numToMove = View::getInt();

		//if they pick a number not between 1 and the number they are allowed to move
		if (numToMove < 1 || numToMove > map->getCountryArmies(sourceCountry) - 1){
			View::inform("Invalid input.");
			View::prompt("Please enter a number between 1 and " + intToString(map->getCountryArmies(sourceCountry) - 1) + ".");
		}
	} while (numToMove < 1 || numToMove > map->getCountryArmies(sourceCountry) - 1);

	//update the two countries army counts
	map->setCountryArmies(sourceCountry, map->getCountryArmies(sourceCountry) - numToMove, false);
	map->setCountryArmies(destinationCountry, map->getCountryArmies(destinationCountry) + numToMove);

	View::inform(intToString(numToMove) + " armies moved from " + sourceCountry + " to " + destinationCountry + ".");
}

int Game::getCardsExchange(int playerNum)
{
	int infantry = playerArray[playerNum]->getCards()[0]->getQuantity();
	int cavalry = playerArray[playerNum]->getCards()[1]->getQuantity();
	int artillery = playerArray[playerNum]->getCards()[2]->getQuantity();

	int armiesAdded = 0;

	std::cout << "You have " << infantry << " infantry cards, " << cavalry << " calavry cards, and " << artillery << " artillery cards" << std::endl;

	if (playerArray[playerNum]->canExchangeCards())
	{
		std::cout << "You can exchange 3 cards of the same type or three cards of all different types for armies. " << std::endl;
		armiesAdded = exchangeCards(getExchangeChoices(playerNum), playerNum);
	}
	else
	{
		std::cout << "You are not eligible to exchange cards for armies. " << std::endl;
	}

	return armiesAdded;
}

int Game::exchangeCards(int* choices, int playerNum)
{
	static int numOfArmiesExchange = 5;//local static incremented by 5 every time the function is called by any player - the requirement to update the value is met; be careful if change it
	int localNumOfArmiesExchanged = numOfArmiesExchange;
	for (int i = 0; i < 3; i++)
	{
		playerArray[playerNum]->getCards()[i]->decrementQuantity(choices[i]);
	}
	numOfArmiesExchange += 5;
	return localNumOfArmiesExchanged;
}

int* Game::getExchangeChoices(int playerNum)
{
	int* choices = new int[3];
	std::fill(choices, choices + 3, 0);

	std::cout << "You need to choose 3 cards of the same type or three cards of all different types to exchange for armies. " << std::endl;

	if (playerArray[playerNum]->getTotalCards() >= 5)
	{
		std::cout << "You have more than 5 cards. You must exchange three of them for armies. " << std::endl;
		int sumOfChoices = choices[0] + choices[1] + choices[2];
		do
		{
			//check if a player will exchange all cards of the same type
			for (int i = 0; i < 3; i++)
			{
				if (playerArray[playerNum]->getCards()[i]->getQuantity() > 3)
				{
					std::cout << "You have more than 3 cards of " << playerArray[playerNum]->getCards()[i]->getType(i) << ". Would you like to exchange all of them for armies ? (y / n)" << std::endl;
					char decision = 'n';
					do
					{
						if (decision != 'n' && decision != 'y')
						{
							std::cout << "Your choice is invalid. Please choose y or n " << std::endl;
						}
						std::cin >> decision;
					} while (decision != 'n' && decision != 'y');

					if (decision == 'y')
					{
						choices[i] = 3;
						return choices;
					}
					else
					{
						continue;
					}
				}
			}

			for (int i = 0; i < 3; i++)
			{
				if (playerArray[playerNum]->getCards()[i]->getQuantity() > 0)
				{
					char choice;
					std::cout << "You have " << playerArray[playerNum]->getCards()[i]->getQuantity() << " of " << playerArray[playerNum]->getCards()[i]->getType(i) << ". Would you like to use " << playerArray[playerNum]->getCards()[i]->getType(i) << " cards for exchange (y/n): ";
					std::cin >> choice;
					do
					{
						if (choice != 'y' && choice != 'n')
						{
							std::cout << "Your choice is invalid. Please choose y or n " << std::endl;
						}

					} while (choice != 'y' && choice != 'n');

					if (choice == 'y')
					{
						int numChoice = 1;
						do
						{
							if (numChoice < 0 && numChoice > playerArray[playerNum]->getCards()[i]->getQuantity())
							{
								std::cout << "Your choice is invalid " << std::endl;
							}
							std::cout << "How many " << playerArray[playerNum]->getCards()[i]->getType(i) << " cards would you like to exchange? Enter the number between 1 and " << playerArray[playerNum]->getCards()[i]->getQuantity() << std::endl;
							std::cin >> numChoice;
						} while (numChoice < 1 && numChoice > playerArray[playerNum]->getCards()[i]->getQuantity());

						choices[i] = numChoice;
					}
					else
					{
						continue;
					}
				}
			}
			sumOfChoices = choices[0] + choices[1] + choices[2];

			if (sumOfChoices > 3)
			{
				return choices;
			}

		} while (sumOfChoices < 3);
	}
	//check if a player will exchange all cards of the same type
	for (int i = 0; i < 3; i++)
	{
		if (playerArray[playerNum]->getCards()[i]->getQuantity() > 3)
		{
			std::cout << "You have more than 3 cards of " << playerArray[playerNum]->getCards()[i]->getType(i) << ". Would you like to exchange all of them for armies ? (y / n)" << std::endl;
			char decision = 'n';
			do
			{
				if (decision != 'n' && decision != 'y')
				{
					std::cout << "Your choice is invalid. Please choose y or n " << std::endl;
				}
				std::cin >> decision;
			} while (decision != 'n' && decision != 'y');

			if (decision == 'y')
			{
				choices[i] = 3;
				return choices;
			}
			else
			{
				continue;
			}
		}
	}

	for (int i = 0; i < 3; i++)
	{
		if (playerArray[playerNum]->getCards()[i]->getQuantity() > 0)
		{
			char choice;
			std::cout << "You have " << playerArray[playerNum]->getCards()[i]->getQuantity() << " of " << playerArray[playerNum]->getCards()[i]->getType(i) << ". Would you like to use " << playerArray[playerNum]->getCards()[i]->getType(i) << " cards for exchange (y/n): ";
			std::cin >> choice;
			do
			{
				if (choice != 'y' && choice != 'n')
				{
					std::cout << "Your choice is invalid. Please choose y or n " << std::endl;
				}

			} while (choice != 'y' && choice != 'n');

			if (choice == 'y')
			{
				int numChoice = 1;
				do
				{
					if (numChoice < 0 && numChoice > playerArray[playerNum]->getCards()[i]->getQuantity())
					{
						std::cout << "Your choice is invalid " << std::endl;
					}
					std::cout << "How many " << playerArray[playerNum]->getCards()[i]->getType(i) << " cards would you like to exchange? Enter the number between 1 and " << playerArray[playerNum]->getCards()[i]->getQuantity() << std::endl;
					std::cin >> numChoice;
				} while (numChoice < 1 && numChoice > playerArray[playerNum]->getCards()[i]->getQuantity());

				choices[i] = numChoice;
			}
			else
			{
				continue;
			}
		}
	}

	return choices;
}

void Game::reinforce(int playerNum){
	int numToReinforce = 0;
	std::string countryToReinforce;
  
	if (!justLoaded){
	  bonusArmies = 0;
	  //calculate # of reinforcements
	  bonusArmies = map->computeContinentsBonuses(playerNum);
	  bonusArmies += map->countCountriesOwned(playerNum) / 3;
	  bonusArmies += getCardsExchange(playerNum);
	}
	else{
	  justLoaded = false;
	}

	while (bonusArmies > 0){
		//Get the country to be reinforced
		View::inform(currentPlayer->getName() + ", you have " + intToString(bonusArmies) + " armies of reinforcements to distribute.");

		View::prompt("Please input the country you want to reinforce.");
		countryToReinforce = View::getString();

		if (countryExistsAndFriendly(countryToReinforce, playerNum)){
			//Get number of reinforcements to deploy in the selected country
			do{
				View::prompt("Please input the number of armies to add to " + countryToReinforce);
				numToReinforce = View::getInt();
				if (numToReinforce < 1 || numToReinforce > bonusArmies){
					View::inform("Invalid input, please enter a number between 1 and " + intToString(bonusArmies));
				}
			} while (numToReinforce < 1 || numToReinforce > bonusArmies);

			//put the number of armies specified in the country specifeid and decrement the number of bonus armies
			map->setCountryArmies(countryToReinforce, map->getCountryArmies(countryToReinforce) + numToReinforce);
			bonusArmies -= numToReinforce;
			View::inform(countryToReinforce + " reinforced with " + intToString(numToReinforce) + " armies and now has " + intToString(map->getCountryArmies(countryToReinforce)) + " armies.");
		}
	}
}


void Game::attack(int playerNum){
	//std::string inString = "f";
	std::string attackingCountry;
	std::string defendingCountry;
	std::list<std::string> enemyNeighbourList;


	//GET THE ATTACKING COUNTRY************************************************************   
	do{
		do{

			View::prompt("Choose a country you wish to launch your attack from.");
			attackingCountry = View::getString();

			//if country doesn't have enough armies to attack another country
			if (map->countryExists(attackingCountry) && map->getCountryOwnerIndex(attackingCountry) == playerNum && map->getCountryArmies(attackingCountry) < 2){
				View::inform(attackingCountry + " does not have enough armies to launch an attack.");
			}

		} while (!(countryExistsAndFriendly(attackingCountry, playerNum) && map->getCountryArmies(attackingCountry) > 1));

		enemyNeighbourList = map->getEnemyNeighbours(attackingCountry, playerNum);

		//it is possible that a country has nothing to attack; for example when a country is in between friendly countries
		//the statement below avoids such possiblity
		if (enemyNeighbourList.empty()){
			View::inform(attackingCountry + " has no enemy neighbours to attack.");
		}

	} while (enemyNeighbourList.empty());

	//GET THE DEFENDING COUNTRY************************************************************
	//output all available countries to attack
	View::inform(attackingCountry + "'s enemy neighbours are:");
	outputCountryList(enemyNeighbourList);

	//Ask for user input and validates to see whether player chose an enemy country       
	do{
		//get country choice and error check
		View::prompt("Choose the country you want to attack");
		defendingCountry = View::getString();

		//if you selected a country that isn't a neighbour
		if (!map->countryExists(defendingCountry)){
			View::inform(defendingCountry + " does not exist.");
		}
		//if you selected a friendly country to attack
		else if (map->getCountryOwnerIndex(defendingCountry) == playerNum)
		{
			View::inform("Invalid choice: " + defendingCountry + " already belongs to you.");
		}
		else if (!listContains(enemyNeighbourList, defendingCountry)){
			View::inform("Invalid choice: " + defendingCountry + " is not a neighbour of " + attackingCountry + ".");
		}
	} while (!(map->countryExists(defendingCountry) && map->getCountryOwnerIndex(defendingCountry) != playerNum &&
		listContains(enemyNeighbourList, defendingCountry)));

	battle(attackingCountry, defendingCountry);



}//end ATTACK function

//Pre: attackingCountry and defendingCountry are validly selected countries to do battle.
//DO BATTLE**************************************************************
void Game::battle(std::string attackingCountry, std::string defendingCountry)
{
	// Boolean variable which will verify whether the attacker wants to keep attacking, it's gonna be used at very end of the while loop
	bool continueBattle = true;
	int lastAttackDice;
	int attackingArmies = map->getCountryArmies(attackingCountry);
	int defendingArmies = map->getCountryArmies(defendingCountry);
	std::string outString;
	std::string inString;
	int inInt;

	View::inform("\n" + attackingCountry + " attacks " + defendingCountry + " with " + intToString(attackingArmies) + " armies against " + intToString(defendingArmies) + " armies.");

	// This while loop is placed to verify whether the attacker wants to keep attacking in case the battle phase is finished and no one won yet

	do{
		//function to randomly generates the dices
		dice->roll_dice(attackingArmies, defendingArmies);

		//BATTLE DAMAGES CALCULATIONS--------------------------------------------------------------
		//function to determine the fightning phase, who wins and who losses

		outString = "";
		if (attackingArmies >1 && defendingArmies > 1){
			if (dice->getFirstAttackDie() > dice->getFirstDefendDie()){
				outString = "The attacker won ";
				defendingArmies--;
				playerArray[map->getCountryOwnerIndex(attackingCountry)]->setBattlesWon();
				playerArray[map->getCountryOwnerIndex(defendingCountry)]->setBattlesLost();
			}
			else{
				outString = "The attacker lost ";
				attackingArmies--;
				playerArray[map->getCountryOwnerIndex(defendingCountry)]->setBattlesWon();
				playerArray[map->getCountryOwnerIndex(attackingCountry)]->setBattlesLost();
			}
			outString += "the first attack " + intToString(dice->getFirstAttackDie()) + ":" + intToString(dice->getFirstDefendDie());

			if (dice->getSecondAttackDie() > dice->getSecondDefendDie()){
				outString += "\nThe attacker won ";
				defendingArmies--;
				playerArray[map->getCountryOwnerIndex(attackingCountry)]->setBattlesWon();
				playerArray[map->getCountryOwnerIndex(defendingCountry)]->setBattlesLost();
			}
			else{
				outString += "\nThe attacker lost ";
				attackingArmies--;
				playerArray[map->getCountryOwnerIndex(defendingCountry)]->setBattlesWon();
				playerArray[map->getCountryOwnerIndex(attackingCountry)]->setBattlesLost();
			}
			outString += "the second attack " + intToString(dice->getSecondAttackDie()) + ":" + intToString(dice->getSecondDefendDie());
			lastAttackDice = dice->getSecondAttackDie();
		}
		else{
			if (dice->getFirstAttackDie() > dice->getFirstDefendDie()){
				outString = "The attacker won ";
				defendingArmies--;
				playerArray[map->getCountryOwnerIndex(attackingCountry)]->setBattlesWon();
				playerArray[map->getCountryOwnerIndex(defendingCountry)]->setBattlesLost();
			}
			else{
				outString = "The attacker lost ";
				attackingArmies--;
				playerArray[map->getCountryOwnerIndex(defendingCountry)]->setBattlesWon();
				playerArray[map->getCountryOwnerIndex(attackingCountry)]->setBattlesLost();
			}
			outString += "the attack " + intToString(dice->getFirstAttackDie()) + ":" + intToString(dice->getFirstDefendDie());
			lastAttackDice = dice->getFirstAttackDie();
		}


		View::inform(outString);

		//output updated army counts 
		View::inform(attackingCountry + ": " + intToString(attackingArmies) + " armies remaining \n" + defendingCountry + ": " + intToString(defendingArmies) + " armies remaining");


		//BATTLE OUTCOME CALCULATIONS--------------------------------------------------------------
		outString = "";
		if (attackingArmies <= 1)    //Verify whether the attacker has no army left
		{
			continueBattle = false;
			outString += "\n" + attackingCountry + " does not have enough armies to continue the attack";

		}
		else if (defendingArmies <= 0)    //Verify whether the defender has no army left
		{
			continueBattle = false;
			outString += "\nYou have conquered " + defendingCountry + ".";
			outString += "\nYou must settle at least " + intToString(lastAttackDice) + " armies in this newly conquered territory.";

			//The attacker won, so now he must deploy his army
			if (attackingArmies > lastAttackDice)
			{
				//Validates to see whether the player has deployed a valid amount of army to conquered country
				do{
					View::inform(outString);
					outString = "";
					View::prompt("How many armies would you like to settle in " + defendingCountry + "?");
					inInt = View::getInt();
					//Asking the user again to deploy the right amount
					if (inInt > attackingArmies - 1 || inInt < lastAttackDice){
						View::inform("Invalid input. Need to add between " + intToString(lastAttackDice) + " and " + intToString(attackingArmies - 1) + " armies.");
					}
				} while (inInt > attackingArmies - 1 || inInt < lastAttackDice);

				//Update the number of armies in the two countries
				defendingArmies = inInt;
				attackingArmies -= inInt;

				outString = intToString(inInt) + " armies moved from " + attackingCountry + " to " + defendingCountry + ".\n";
			}
			else    // This is when the attacker won but doesn't have enough army greater than his previous rolled dice
			{
				outString += "\nBecause you have only " + intToString(attackingArmies) + " armies left in " + attackingCountry + ", you automatically settle " + intToString(attackingArmies - 1) + " in " + defendingCountry + ".";

				//Update the army counts
				defendingArmies = attackingArmies - 1;
				attackingArmies = 1;
			}

			//change the owner of the country
			map->setCountryOwnerIndex(defendingCountry, map->getCountryOwnerIndex(attackingCountry), false);
			//transfer cards if required
			int playerLostInd = map->getCountryOwnerIndex(defendingCountry);
			int playerWonInd = map->getCountryOwnerIndex(attackingCountry);
			if (map->countCountriesOwned(playerLostInd) < 1)
			{
				playerArray[playerLostInd]->transferCards(playerArray[playerWonInd]);
			}
		}

		//If both sides attacker and defender still have armies left after the battle phase then the attacker will receive the choice to continue attacking or stop
		else
		{
			View::prompt("Would you like to continue the attack (y/n)?");
			inString = View::getString();
			continueBattle = inString.compare("n") != 0;
		}


	} while (continueBattle);




	//Update the number of armies in the two countries before terminating the function
	map->setCountryArmies(defendingCountry, defendingArmies, false);
	map->setCountryArmies(attackingCountry, attackingArmies);


	//Output summary of the battle via outString
	outString += "\nAt the end of battle:";
	outString += "\n  " + attackingCountry + " has " + intToString(attackingArmies) + " armies";
	outString += "\n  " + defendingCountry + " has " + intToString(defendingArmies) + " armies";
	outString += "\n  " + defendingCountry;

	if (map->getCountryOwnerIndex(defendingCountry) == map->getCountryOwnerIndex(attackingCountry)){
		outString += " now ";
	}
	else
	{
		outString += " still ";
	}

	for (int i = 0; i < numOfPlayers; i++){
		if (playerArray[i]->getPlayerIndex() == map->getCountryOwnerIndex(defendingCountry)){
			defendingPlayer = playerArray[i];
		}
	}

	outString += " belongs to " + defendingPlayer->getName() + "\n";
	totalBattles++;
	View::inform(outString);

}//END battle function



Player* Game::findPlayerByIndex(int i){
	Player* _player = NULL;
	for (int x = 0; x < numOfPlayers; x++){
		if (playerArray[x]->getPlayerIndex() == i){
			_player = playerArray[x];
			break;
		}
	}
	return _player;
}

void Game::displayStatistics(){
	for (int x = 0; x < numOfPlayers; x++){
		//various numbers used to calculate the percentages
		std::string countriesOwned = intToString(playerArray[x]->getNumCountriesOwned());
		std::string armiesOwned = intToString(playerArray[x]->getNumArmiesOwned());
		std::string playerName = playerArray[x]->getName();
		int totalCountries = map->getCountryCount();
		int battlesWon = playerArray[x]->getBattlesWon();
		int battlesLost = playerArray[x]->getBattlesLost();
		int totalBattles = playerArray[x]->getBattlesWon() + playerArray[x]->getBattlesLost();
		//scale is used to format output to 2 digits
		double scale = 0.01;
		double percentCountriesOwned = (double(playerArray[x]->getNumCountriesOwned()) / double(totalCountries) * 100);
		double roundedCountriesOwned = floor(percentCountriesOwned / scale + 0.5)*scale;


		//calculates percentage of battles won, not rounded 
		double percentBattlesWon = (double(battlesWon) / double(totalBattles) * 100);
		double roundedBattlesWon = 0;
		//roundedBattlesWon is implemented this way to prevent an output error
		//rounds the percentage of battles won
		if (totalBattles != 0){
			roundedBattlesWon = floor(percentBattlesWon / scale + 0.5)*scale;
		}


		//convert double to string for countries owned
		std::ostringstream countries;
		countries << roundedCountriesOwned;
		std::string str = countries.str();

		//convert double to string for battles won
		std::ostringstream battles;
		battles << roundedBattlesWon;
		std::string str2 = battles.str();

		//output both percentage and the numbers
		View::inform("--------------------------------------");
		View::inform(playerName + " owns " + armiesOwned + " armies across " + countriesOwned + " countries.");
		View::inform(playerName + " owns " + str + "% of the map (" + countriesOwned + "/" + intToString(totalCountries) + ").");
		View::inform(playerName + " won " + intToString(battlesWon) + " battles and lost " + intToString(battlesLost) + " battles.");
		View::inform(playerName + " won " + str2 + "% of the battles (" + intToString(battlesWon) + "/" + intToString(totalBattles) + ").");
	}
}
void Game::handleCards(int playerNum)
{
	if (playerArray[playerNum]->getHasConquered())
	{
		playerArray[playerNum]->addCard();
	}
}

void Game::saveGame(){
  GameBuilderDirector gbd;
  int inInt;
  do{
    View::prompt("Please input a save slot (0-9)");
    inInt = View::getInt();
  }while(inInt < 0 || inInt > 9);
  
  GameBuildSave * gbs = new GameBuildSave(inInt, this);
  gbd.setGameBuilder(gbs);
  gbd.constructGame();
  delete gbs;
  View::inform("Game Saved Successfully\n");
}
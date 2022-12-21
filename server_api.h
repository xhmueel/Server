#ifndef SERVER_API_H
#define SERVER_API_H

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <stdio.h>
#include <vector>
#include <iostream>
#include <sys/stat.h>
#include <time.h>

//create verbose vector string and put "n" as default
extern std::string verbose;
//GS port
extern std::string port;
//name of the words file
extern std::string word_file;
//file descriptor of word_file
extern std::FILE *word_file_fd;


//parse command line arguments ./GS word_file [-p GS_port] [-v]
void parse_init_args(int argc, char *argv[]);

//check if GSport is valid
int portIsValid(std::string port);

//treat SIGINT signal
void SIGINT_handler(int sig);

//treat UDP requests
void handleUDPrequests(std::string port);

//udp switch
void udpSwitch(char *buffer_send, std::string vec_receive);

// returns number of words in str
int countWords(std::string str);

//parse arguments in string by whitespace
std::vector<std::string> parseArgs(std::string line);

//check if player has ongoing game, has if game exists but no plays were made, return word to be guessed
std::string hasOngoingGame(std::string PLID);

//check if is a PLID
int isPLID(std::string PLID);

//create new game
std::string createNewGame(std::string PLID);

//get random word from word_file
void getSequencialWordandHint(char* random_word);

//get word lenght and max_errors
std::string getWordLenghtandMaxErrors(std::string str);

//check if string is a letter
int isLetter(std::string str);

//check if string str is a number between 0 and 9
int isNumber(std::string str);

//play letter from game PLID
void playLetter(std::string PLID, std::string letter, std::string nr_trials, char* buffer_send);

//get word max_errors
int getWordMaxErrors(char* word);

//check if word is a word
int isWord(std::string str);

//play word from game PLID
void playWord(std::string PLID, std::string word, std::string nr_trials, char* buffer_send);

//change file name and directory when game ends
void gameEnds(char* filename, std::string PLID, char* end_mode);

//quit game
void quitGame(std::string PLID, char *buffer_send);

//treat SIGINT signal
void SIGINT_handler(int sig);

//treat SIGSEGV signal
void SIGSEGV_handler(int sig);

//delete GAME_(plid).txt file
void deleteGame(std::string PLID);

//delete GAME/(plid) directory
void deletePlayerDir(std::string PLID);



#endif
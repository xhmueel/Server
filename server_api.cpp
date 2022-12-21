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
#include "server_api.h"

//default port of the group 16
#define PORT "58016" 
#define MAX_BUFFER 128

#define FALSE 0
#define TRUE 1
#define MAX_WORD_LENGTH 30
#define MIN_WORD_LENGTH 3


using namespace std;


//server to treat udp requests
void handleUDPrequests(string port){

    int fd_udp,errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints,*res;
    struct sockaddr_in addr;
    char buffer_rec[MAX_BUFFER];
    char buffer_send[MAX_BUFFER];

    //pass port to char*
    char port_char[7];
    strcpy(port_char, port.c_str());

    fd_udp = socket(AF_INET,SOCK_DGRAM,0); //UDP socket

    if(fd_udp == -1){
        //error in crceating socket
        printf("Error in creating socket");
        exit(EXIT_FAILURE);
    }

    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET; // IPv4
    hints.ai_socktype=SOCK_DGRAM; // UDP socket
    hints.ai_flags=AI_PASSIVE;

    errcode = getaddrinfo(NULL, PORT,&hints,&res);
    if(errcode!=0){
        //error in getaddrinfo
        printf("Error in getaddrinfo");
        exit(EXIT_FAILURE);
    }

    n = bind(fd_udp, res->ai_addr, res->ai_addrlen);
    if(n == -1){
        //error in bind
        printf("Error in bind");
        exit(EXIT_FAILURE);
    }

    while (1){
        addrlen = sizeof(addr);
        memset(buffer_rec,'\0', MAX_BUFFER);
        n = recvfrom(fd_udp, buffer_rec, 128, 0, (struct sockaddr*)&addr, &addrlen);
        if(n == -1){
            //error in recvfrom
            printf("Error in recvfrom");
            exit(EXIT_FAILURE);
        }
        
        //treat request
        memset(buffer_send,'\0', MAX_BUFFER);
        string str_receive = buffer_rec;
        udpSwitch(buffer_send, str_receive);

        n = sendto(fd_udp, buffer_send, n, 0, (struct sockaddr*)&addr, addrlen);
        if(n == -1){
            //error in sendto
            printf("Error in sendto");
            exit(EXIT_FAILURE);
        }
    }

    freeaddrinfo(res);
    close(fd_udp);
}

//udp switch
void udpSwitch(char *buffer_send, string buffer_rec){

    //pass verbose to char*
    char verbose_char[2];
    strcpy(verbose_char, verbose.c_str());
    verbose_char[1] = '\0';

    //parse message from client
    vector<string> parsed_message = parseArgs(buffer_rec);

    //switch to see what request client made
    if(parsed_message[0].compare("SNG") == 0){
        //request to start new game
        int isplid= isPLID(parsed_message[1]);
        if(isplid == FALSE){
            //PLID is not valid
            strcpy(buffer_send,"SNG ERR\n");
            return;
        }
        //if verbose mode
        parsed_message[1].erase(parsed_message[1].size()-1);
        if(strcmp(verbose_char, "s") == 0){
            printf("(%s, SNG)\n", parsed_message[1].c_str());
        }
        //start new game
        string res = hasOngoingGame(parsed_message[1]);
        //plid doens't have ongoing game
        if(res == "FALSE"){
            //create new game
            string word = createNewGame(parsed_message[1]);
            //send message "RSG OK (number of letter) (max_errors)"
            strcpy(buffer_send,"RSG OK ");
            strcat(buffer_send, getWordLenghtandMaxErrors(word).c_str());
            strcat(buffer_send, "\n\0");
            return;
        }
        else if(res == "TRUE"){
            //player has already ongoing game
            strcpy(buffer_send,"RSG NOK\n\0");
            return;
        }
        else{
            //game exists but player has made no moves
            //send message "RSG OK (number of letter) (max_errors)"
            strcpy(buffer_send,"RSG OK ");
            strcat(buffer_send, getWordLenghtandMaxErrors(res).c_str());
            strcat(buffer_send, "\n\0");
            return;
        }
    }
    //request to play letter
    else if(parsed_message[0].compare("PLG") == 0){
        //play letter
        string PLID = parsed_message[1];
        if(isPLID(PLID) == FALSE){
            //PLID is not valid
            strcpy(buffer_send,"RLG ERR\n\0"); 
            return;
        }
        //if verbose mode
        if(strcmp(verbose_char, "s") == 0){
            printf("(%s, PLG)\n", parsed_message[1].c_str());
        }
        string letter = parsed_message[2];
        if(isLetter(letter) == FALSE){
            //letter is not valid
            strcpy(buffer_send,"RLG ERR\n\0");
            return;
        }
        string nr_trials = parsed_message[3];
        //take \n out of nr_trials and add \0
        nr_trials = nr_trials.substr(0, nr_trials.size()-1);
        nr_trials = nr_trials + "\0";

        if(isNumber(nr_trials) == FALSE){
            //nr_trials is not valid
            strcpy(buffer_send,"RLG ERR\n\0");
            return;
        }
        
        //play letter
        playLetter(PLID, letter, nr_trials, buffer_send);
        return;

    }
    //request to play word
    else if(parsed_message[0].compare("PWG")==0){
        //play word
        string PLID = parsed_message[1];
        if(isPLID(PLID) == FALSE){
            //PLID is not valid
            strcpy(buffer_send,"RWG ERR\n\0");
            return;
        }
        //if verbose mode
        if(strcmp(verbose_char, "s") == 0){
            printf("(%s, PWG)\n", parsed_message[1].c_str());
        }
        string word = parsed_message[2];
        if(isWord(word) == FALSE){
            //word is not valid
            strcpy(buffer_send,"RWG ERR\n\0");
            return;
        }
        string nr_trials = parsed_message[3];
        //take \n out of nr_trials and add \0
        nr_trials = nr_trials.substr(0, nr_trials.size()-1);
        nr_trials = nr_trials + "\0";

        if(isNumber(nr_trials) == FALSE){
            //nr_trials is not valid
            strcpy(buffer_send,"RWG ERR\n\0");
            return;
        }

        //play the word
        playWord(PLID, word, nr_trials, buffer_send);
        return;

    }
    else if(parsed_message[0].compare("QUT")==0){     //ongoing game needs moves?
        //quit/exit game
        string PLID = parsed_message[1];
        
        if(isPLID(PLID) == FALSE){
            //PLID is not valid
            strcpy(buffer_send,"RQT ERR\n\0");
            return;
        }
        //if verbose mode
        parsed_message[1].erase(parsed_message[1].size()-1);
        if(strcmp(verbose_char, "s") == 0){
            printf("(%s, QUT)\n", parsed_message[1].c_str());
        }
        //quit game
        quitGame(PLID, buffer_send);
        
    }
    //delete game_(plid).txt file
    else if(parsed_message[0].compare("KILLGAME")==0){
        string PLID = parsed_message[1];
        if(isPLID(PLID) == FALSE){
            //PLID is not valid
            strcpy(buffer_send,"KILLGAME ERR\n\0");
            return;
        }
        //delete game_(plid).txt file
        deleteGame(PLID);

    }
    //delete GAMES/(plid) directory
    else if(parsed_message[0].compare("KILLPDIR")==0){
        string PLID = parsed_message[1];
        if(isPLID(PLID) == FALSE){
            //PLID is not valid
            strcpy(buffer_send,"KILLPDIR ERR\n\0");
            return;
        }
        //delete GAMES/(plid) directory
        deletePlayerDir(PLID);
    }
    /* else if(strcmp(parsed_message[0],"REV")==0){
        //debug command
        exit(EXIT_SUCCESS);
    } */
    else{
        //error
        strcpy(buffer_send,"ERR\n\0");
        return;
    }
    
}


// returns number of words in str
int countWords(string str){
    //counts number of words in string str
    int count = 0;
    for (int i = 0; i < str.size(); i++){
        if (str[i] == ' ' || str[i] == '\t')
            count++;
    }
    return count + 1;


   /*  int state = 0;
    int wc = 0; // word count

    // Scan all characters one by one
    while (*str)
    {
        if (*str == '\n')
            break;

        // If next character is a separator, set the
        // state as OUT
        else if (*str == ' ' || *str == '\t')
            state = 0;

        // If next character is not a word separator and
        // state is OUT, then set the state as IN and
        // increment word count
        else if (state == 0)
        {
            state = 1;
            ++wc;
        }

        // Move to next character
        ++str;
    }

    return wc; */
}

//parse arguments in string by spaces
vector<string> parseArgs(string line){

    int inputc = countWords(line);   
    vector<string> parsed_message;
    string parsed_line;

    //parse message by spaces
    for(int i = 0; i < line.size(); i++){
        if(line[i] != ' '){
            parsed_line += line[i];
        }
        else{
            parsed_message.push_back(parsed_line);
            parsed_line = "";
        }
    }
    parsed_message.push_back(parsed_line);

    return parsed_message;

}


//check if PLID is valid
int isPLID(string plid){
    //pass string to int
    int PLID = stoi(plid);
    //check if PLID is 6 digits long
    if(PLID < 100000 || PLID > 999999){
        return FALSE;
    }
    return TRUE;
}

//check if player has ongoing game
string hasOngoingGame(string plid){
    //pass plid to a char*
    char PLID_arr[7];
    strcpy(PLID_arr,plid.c_str());
    //strcat(PLID_arr,"\0");
    PLID_arr[6] = '\0';

    //check if file GAME_(plid).txt exists
    char filename[16];
    strcpy(filename,"GAME_");
    strcat(filename,PLID_arr);
    strcat(filename,".txt");
    strcat(filename,"\0");

    FILE *fp = fopen(filename,"r");
    if(fp == NULL){
        //fclose(fp);
        return "FALSE";
    }
    //check if file only has 1 line, and stores it in a string
    char buffer[MAX_BUFFER];
    fgets(buffer,MAX_BUFFER,fp);
    //only get word to be guessed and not hint filename
    for(int i = 0; i < MAX_BUFFER; i++){
        if(buffer[i] == ' '){
            buffer[i] = '\0';
            break;
        }
    }

    string str_buffer = buffer;

    if(fgets(buffer,MAX_BUFFER,fp)){
        //file has more than 1 line
        fclose(fp);
        return "TRUE";
    }

    fclose(fp);
    return str_buffer;

}

//create new game
string createNewGame(string plid){
    //pass plid to a char*
    char PLID_arr[7];
    strcpy(PLID_arr,plid.c_str());
    //strcat(PLID_arr,"\0");
    PLID_arr[6] = '\0';

    //create file
    char filename[16];
    strcpy(filename,"GAME_");
    strcat(filename,PLID_arr);
    strcat(filename,".txt");
    strcat(filename,"\0");

    //open GAME_(plid).txt
    FILE *fp = fopen(filename,"w");
    if(fp == NULL){
        //error
        //fclose(fp);
        //exit(EXIT_FAILURE);
        return "no";
    }

    //write to file
    char random_word[MAX_BUFFER];

    //memset(random_word,'\0',MAX_BUFFER);
    getSequencialWordandHint(random_word);
    fputs(random_word,fp);
    //only get word to be guessed and not hint filename
    for(int i = 0; i < MAX_BUFFER; i++){
        if(random_word[i] == ' '){
            random_word[i] = '\0';
            break;
        }
    }
    string str_random_word = random_word;
    fclose(fp);
    return str_random_word;
}


//get sequencial word from word_file
void getSequencialWordandHint(char* random_word){
    //transform word_file from string to char*
    /* char word_file_char[MAX_BUFFER];
    strcpy(word_file_char, word_file.c_str());
    //strcat(word_file_char,"\0");
    word_file_char[word_file.size()] = '\0';


    if(word_file_fd == NULL){
        //open word_file to get sequencial word
        FILE *word_file_fd = fopen(word_file_char,"r");
        if(word_file_fd == NULL){
            //error
            exit(EXIT_FAILURE);
        }
    } */
   
    //get next word from file
    char line[MAX_BUFFER];
    if(fgets(line,MAX_BUFFER,word_file_fd) == NULL){
        //if end of file, go back to beginning
        fseek(word_file_fd,0,SEEK_SET);
        fgets(line,MAX_BUFFER,word_file_fd); //?
    }

    //only get the word, and not the hint filename
    for(int i = 0; i < MAX_BUFFER; i++){
        if(line[i] == ' '){
            line[i] = '\0';
            break;
        }
    }

    //get 

    //get number of lines in file
    /* int line_count = 0;
    char line[MAX_BUFFER];
    while(fgets(line,MAX_BUFFER,fp)){
        line_count++;
    }

    //get random line
    int random_line = rand() % line_count;
    fseek(fp,0,SEEK_SET);
    for(int i = 0; i < random_line; i++){
        fgets(line,MAX_BUFFER,fp);
    } */

    //only get the word, and not the hint filename
    /* for(int i = 0; i < MAX_BUFFER; i++){
        if(line[i] == ' '){
            line[i] = '\0';
            break;
        }
    } */

    strcpy(random_word,line);
}

//get word max errors
int getWordMaxErrors(char* word){
    int len = strlen(word);

    if(len <= 6){
        return 7;
    }
    else if(len > 6 && len <= 10){
        return 8;
    }
    else{
        return 9;
    }

}


//get word lenght and max_errors
string getWordLenghtandMaxErrors(string str){
    //get str lenght
    int str_lenght = str.size();
    //get max_errors
    string max_errors;
    if(str_lenght <= 6){
        max_errors = "7";
    }
    else if(str_lenght > 6 && str_lenght <= 10){
        max_errors = "8";
    }
    else{
        max_errors = "9";
    }

    string str_lenght_max_errors = to_string(str_lenght) + " " + max_errors;
    return str_lenght_max_errors;
}

//check if string is letter
int isLetter(string str){
    if(str.size() != 1){
        return FALSE;
    }
    if((str[0] < 'A' || str[0] > 'Z') && (str[0] < 'a' || str[0] > 'z')){
        return FALSE;
    }
    return TRUE;
}

//check if string str is a number
int isNumber(string str){
    for(int i = 0; i < str.size(); i++){
        if(str[i] < '0' || str[i] > '9'){
            return FALSE;
        }
    }
    return TRUE;
}


//play letter from game PLID
void playLetter(string PLID, string letter, string nr_trials, char* buffer_send){
    //pass plid to a char*
    char PLID_arr[7];
    strcpy(PLID_arr,PLID.c_str());
    PLID_arr[6] = '\0';

    //pass letter to a char lower and uppercase
    char letter_char = letter[0];
    char letter_lower = tolower(letter_char);
    char letter_upper = toupper(letter_char);
    char letter_arr_lower[2] = {letter_lower,'\0'};
    char letter_arr_upper[2] = {letter_upper,'\0'};

    //pass letter to char*
    char letter_arr[2];
    strcpy(letter_arr,letter.c_str());
    letter_arr[1] = '\0';
    
    //open file
    char filename[16];
    strcpy(filename,"GAME_");
    strcat(filename,PLID_arr);
    strcat(filename,".txt");
    strcat(filename,"\0");

    FILE *fp = fopen(filename,"r+");
    if(fp == NULL){
        //error
        strcpy(buffer_send,"RLG ERR\n\0");
        return; 
    }

    //get word to be guessed
    int len;
    char word_to_be_guessed[MAX_BUFFER];
    fgets(word_to_be_guessed,MAX_BUFFER,fp);
    //only get word to be guessed and not hint filename
    for(int i = 0; i < MAX_BUFFER; i++){
        if(word_to_be_guessed[i] == ' '){
            word_to_be_guessed[i] = '\0';
            len = i;
            break;
        }
    }

    int max_errors = getWordMaxErrors(word_to_be_guessed);

    //read every line of file fp
    char line[MAX_BUFFER];
    char word_guessed[MAX_BUFFER];
    char letter_line_lower[5];
    char letter_line_upper[5];
    int gs_trials = 0;
    
    memset(word_guessed,'_',MAX_BUFFER);
    word_guessed[len] = '\0';

    //build line "T letter" upper case and lower case
    strcpy(letter_line_lower, "T ");
    strcat(letter_line_lower, letter_arr_lower);
    strcat(letter_line_lower, "\n\0");

    strcpy(letter_line_upper, "T ");
    strcat(letter_line_upper, letter_arr_upper);
    strcat(letter_line_upper, "\n\0");


    int is_dup = FALSE;
    int letter_right;
    int wrong_plays = 0;

    //check if letter was already played, get wrong moves and build word_guessed
    while(fgets(line,MAX_BUFFER,fp)){
        gs_trials++;
        letter_right = FALSE;
        //build the guessed word until now
        if((line[0] == 'T') && is_dup == FALSE){
            for(int i = 0; i < MAX_BUFFER; i++){
                //compare line[2] with word_to_be_guessed[i] case insensitive
                if(toupper(line[2]) == word_to_be_guessed[i] || tolower(line[2]) == word_to_be_guessed[i]){
                    letter_right = TRUE;
                    word_guessed[i] = line[2];
                }
                if(word_to_be_guessed[i] == '\0' || word_to_be_guessed[i] == '\n'){
                    word_guessed[i] = '\0';
                    break;
                }
            }
            if(letter_right == FALSE){
                wrong_plays++;
            }
        }
        if((line[0] == 'G') && is_dup == FALSE){
            wrong_plays++;
        }
        if(strcmp(line,letter_line_lower) == 0 || strcmp(line,letter_line_upper) == 0){
            is_dup = TRUE;
        }
    }

    //check if trials send by client are the same as the ones in the file
    int nr_trials_int = stoi(nr_trials);
    if((gs_trials + 1) != stoi(nr_trials)){
        fclose(fp);
        strcpy(buffer_send,"RLG INV ");
        char gs_trials_char[5];
        sprintf(gs_trials_char,"%d",gs_trials);
        strcat(buffer_send, gs_trials_char);
        strcat(buffer_send, "\n\0");
        return;
        //TO DO ( if the player is repeating the last PLG stored at the GS with a different letter;)
    }

    if(is_dup == TRUE){
        //pass gs_trials to char*
        char gs_trials_char[5];
        sprintf(gs_trials_char,"%d",gs_trials);

        fclose(fp);
        strcpy(buffer_send,"RLG DUP ");
        //sends trials done (not counting the current one)
        strcat(buffer_send, gs_trials_char);  
        strcat(buffer_send, "\n\0");
        return;
    }

    
    //write letter_line to filename at the end of file
    // (TO DO) what if the message ACK gets lost
    fputs(letter_line_upper,fp);
    fclose(fp);

    //check if letter is in word to be guessed
    int letter_in_word = FALSE;
    //string to keep number of occurrences and positions
    int n_occurrences = 0;
    char pos[MAX_BUFFER] = " ";

    //see if letter played is in word to be guessed
    for(int i = 0; i < MAX_BUFFER; i++){
        if(word_to_be_guessed[i] == '\0'){
            word_guessed[i] = '\0';
            break;
        }
        //compare letter[0] with word_to_be_guessed[i] case insensitive
        if(toupper(letter[0]) == word_to_be_guessed[i] || tolower(letter[0]) == word_to_be_guessed[i]){
            //increment number of occurrences
            n_occurrences++;
            //add position to pos
            char pos_aux[5];
            sprintf(pos_aux,"%d",i + 1);
            strcat(pos,pos_aux);
            strcat(pos," ");

            word_guessed[i] = word_to_be_guessed[i];
            letter_in_word = TRUE;
        }
    }
    

    //letter not in word
    if(letter_in_word == FALSE){
        wrong_plays++;
        //increment nr_trials
        //(TO DO) don´t increment trials if message ACK gets lost
        nr_trials_int++;
        nr_trials = to_string(nr_trials_int);
        //check if max_errors == max_errors
        if(max_errors == wrong_plays){
            //game over
            strcpy(buffer_send,"RLG OVR ");
            char nr_trials_char[5];
            sprintf(nr_trials_char,"%d",nr_trials_int);
            strcat(buffer_send, nr_trials_char);
            strcat(buffer_send, "\n\0");
            //end game and treat game file
            char f[2] = "F";
            gameEnds(filename, PLID, f); //TO DO create score file
            return;
        }
        else if(max_errors > wrong_plays){
            //game not over
            strcpy(buffer_send,"RLG NOK ");
            char nr_trials_char[5];
            sprintf(nr_trials_char,"%d",nr_trials_int);
            strcat(buffer_send, nr_trials_char);
            strcat(buffer_send, "\n\0");
            return;
        }
    }
    
    //check if letter guessed the word
    for(int i = 0; i < MAX_BUFFER; i++){
        if(word_guessed[i] == '\0'){
            //letter guessed the word
            word_guessed[i] = '\0';
            strcpy(buffer_send,"RLG WIN ");
            strcat(buffer_send, nr_trials.c_str());
            strcat(buffer_send, "\n\0");
            //end game and treat game file
            char w[2] = "W";
            gameEnds(filename, PLID, w); //TO DO create score file
            return;
        }
        if(word_guessed[i] == '_'){
            //letter didn´t guess the word
            char response[MAX_BUFFER];
            strcpy(response,"RLG OK ");
            strcat(response,nr_trials.c_str());
            strcat(response," ");

            char nr_occ[2];
            sprintf(nr_occ,"%d",n_occurrences);
            strcat(response,nr_occ);
            strcat(response,pos);
            strcat(response,"\n\0");
            strcpy(buffer_send,response);

            return;

        }
    }

    

}

//check if word is a valid word
int isWord(string word){
    if((word.length() > MAX_WORD_LENGTH) || (word.length() < MIN_WORD_LENGTH)){
        return FALSE;
    }

    for(int i = 0; i < word.length(); i++){
        if((word[i] < 'A' || word[i] > 'Z') && (word[i] < 'a' || word[i] > 'z')){
            return FALSE;
        }
    }

    return TRUE;
}


//play word
void playWord(string PLID, string word, string nr_trials, char* buffer_send){

    //pass plid to a char*
    char PLID_arr[7];
    strcpy(PLID_arr,PLID.c_str());
    //strcat(PLID_arr,"\0");
    PLID_arr[6] = '\0';

    //open file
    char filename[16];
    strcpy(filename,"GAME_");
    strcat(filename,PLID_arr);
    strcat(filename,".txt");
    strcat(filename,"\0");

    FILE *fp = fopen(filename,"r+");
    if(fp == NULL){
        //error
        strcpy(buffer_send,"RWG ERR\n\0"); // \n missing at the end?
        return; 
    }

    //get word to be guessed
    char word_to_be_guessed[MAX_BUFFER];
    fgets(word_to_be_guessed,MAX_BUFFER,fp);
    //only get word to be guessed and not hint filename
    for(int i = 0; i < MAX_BUFFER; i++){
        if(word_to_be_guessed[i] == ' '){
            word_to_be_guessed[i] = '\0';
            break;
        }
    }

    //get max errors for word
    int max_errors = getWordMaxErrors(word_to_be_guessed);

    //read every line of file fp
    char line[MAX_BUFFER];
    char letter_line[4];
    int gs_trials = 0;
    int word_is_right = FALSE;
    int letter_right = FALSE;
    int wrong_plays = 0;

    //build line "G word"
    strcpy(letter_line, "G ");
    strcat(letter_line, word.c_str());
    strcat(letter_line, "\n\0");

    //get number of errors
    while(fgets(line,MAX_BUFFER,fp)){
        gs_trials++;
        letter_right = FALSE;
        //see if word is right and get wrong plays up until now
        if((line[0] == 'T')){
            for(int i = 0; i < MAX_BUFFER; i++){
                //compare line[2] with word_to_be_guessed[i] case insensitive
                if(toupper(line[2]) == word_to_be_guessed[i] || tolower(line[2]) == word_to_be_guessed[i]){
                    letter_right = TRUE;
                }
                if(word_to_be_guessed[i] == '\0' || word_to_be_guessed[i] == '\n'){
                    break;
                }
            }
            if(letter_right == FALSE){
                wrong_plays++;
            }
        }
        if(line[0] == 'G'){
            wrong_plays++;
        }
    }

    //check if trials are correct
    int nr_trials_int = stoi(nr_trials);
    if((gs_trials + 1) != stoi(nr_trials)){
        fclose(fp);
        strcpy(buffer_send,"RWG INV "); 
        char gs_trials_char[5];
        //sends trials done (not including the current one)
        sprintf(gs_trials_char,"%d",gs_trials);
        strcat(buffer_send,gs_trials_char);
        strcat(buffer_send,"\n\0");
        return;
        //TO DO ( if the player is repeating the last PLG stored at the GS with a different letter;)
    }

    //write letter_line to filename at the end of file
    // (TO DO) what if the message ACK gets lost
    fputs(letter_line,fp);
    fclose(fp);

    //check if word is right
    for(int i = 0; i < MAX_BUFFER; i++){

        if((letter_line[i + 2] == '\n') && (word_to_be_guessed[i] == '\0')){
            word_is_right = TRUE;
            break;
        }
        else if((tolower(letter_line[i + 2]) != word_to_be_guessed[i]) && (toupper(letter_line[i + 2]) != word_to_be_guessed[i])){
            break;
        }
        else if(((letter_line[i + 2] == '\n') && (word_to_be_guessed[i] != '\0')) || ((word_to_be_guessed[i] == '\0') && (letter_line[i + 2] != '\n'))){
            word_is_right = FALSE;
            break;
        }
    }

    //check if game is over
    nr_trials_int++;
    if(word_is_right == FALSE){
        wrong_plays++;
        if(max_errors <= wrong_plays){
            //game over
            strcpy(buffer_send,"RWG OVR ");
            char nr_trials_char[5];
            sprintf(nr_trials_char,"%d",nr_trials_int);
            strcat(buffer_send,nr_trials_char);
            strcat(buffer_send,"\n\0");
            //end game and treat game file
            char f[2] = "F";
            gameEnds(filename, PLID, f); //TO DO create score file
            return;
        }
        else{
            //game not over
            strcpy(buffer_send,"RWG NOK ");
            char nr_trials_char[5];
            sprintf(nr_trials_char,"%d",nr_trials_int);
            strcat(buffer_send,nr_trials_char);
            strcat(buffer_send,"\n\0");
            return;
        }
    }
    //word was guessed
    else if(word_is_right == TRUE){
        strcpy(buffer_send,"RWG WIN ");
        char nr_trials_char[5];
        sprintf(nr_trials_char,"%d",nr_trials_int);
        strcat(buffer_send,nr_trials_char);
        strcat(buffer_send,"\n\0");
        //end game and treat game file
        char w[2] = "W";
        gameEnds(filename, PLID, w);  //TO DO create score file
        return;
    }
    
    //word already guessed
    strcpy(buffer_send,"RWG ERR\n\0");
    return;
}

//treat GS game file when game ends
void gameEnds(char* filename, string PLID, char* end_mode){
    //change file directory to GAMES/(PLID)
    char new_dir[13];
    strcpy(new_dir,"GAMES/");
    strcat(new_dir,PLID.c_str());
    //strcat(new_dir,"\0");
    new_dir[12] = '\0';



    //create directory new_dir if it doesn´t exist
    struct stat st = {0};
    if (stat(new_dir, &st) == -1){
        mkdir(new_dir, 0700);
    }

    //change name of filename to YYYYMMDD_HHMMSS_code.txt
    char new_filename[30];
    time_t t = time(NULL);
    struct tm tm = *gmtime(&t);
    strftime(new_filename, sizeof(new_filename), "%Y%m%d_%H%M%S", &tm);
    strcat(new_filename,"_");
    strcat(new_filename,end_mode);
    strcat(new_filename,".txt");
    strcat(new_filename,"\0");
    
    //move new_filename to new_dir
    char new_path[MAX_BUFFER];
    strcpy(new_path,new_dir);
    strcat(new_path,"/");
    strcat(new_path,new_filename);
    strcat(new_path,"\0");


    rename(filename, new_path);

    return;
}

//quit game
void quitGame(string PLID, char *buffer_send){

    //pass plid to a char*
    char PLID_arr[7];
    strcpy(PLID_arr, PLID.c_str());
    PLID_arr[6] = '\0';

    //open file
    char filename[16];
    strcpy(filename,"GAME_");
    strcat(filename,PLID_arr);
    strcat(filename,".txt");
    strcat(filename,"\0");

    FILE *fp = fopen(filename,"r");
    if(fp == NULL){
        //error
        strcpy(buffer_send,"RQT ERR\n\0"); 
        return; 
    }

    //end game and treat game file
    char q[2] = "Q";
    gameEnds(filename, PLID, q); //TO DO create score file

    //send message to client
    strcpy(buffer_send,"RQT OK\n\0");
    //(TO DO) end PLID TCP connections

}

//delete GAME_(plid).txt file
void deleteGame(string PLID){
    //pass plid to a char*
    char PLID_arr[7];
    strcpy(PLID_arr, PLID.c_str());
    PLID_arr[6] = '\0';

    //build string filename
    char filename[16];
    strcpy(filename,"GAME_");
    strcat(filename,PLID_arr);
    strcat(filename,".txt");
    strcat(filename,"\0");

    //delete file
    remove(filename);

    return;
}

//delete GAME/(plid) directory
void deletePlayerDir(string PLID){
    //pass plid to a char*
    char PLID_arr[7];
    strcpy(PLID_arr, PLID.c_str());
    PLID_arr[6] = '\0';

    //build string directory
    char directory[13];
    strcpy(directory,"GAMES/");
    strcat(directory,PLID_arr);
    strcat(directory,"\0");

    //delete directory and its contents (to do)



    return;
}
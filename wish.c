#include<stdio.h>
#include<fcntl.h>
#include<ctype.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>

int mode=0;//0= interactive 1=batch mode
int bufferSize = 512; //size of the buffer, which is used to get our commands
int numPaths=0; //the number of paths that were added by the user
int pathAffected=0; //variable used as a boolean to track whether the path was modified
char paths[512][512]; //a 2d array to store the different paths if more than one was added
int noPath=0; //this is to indicate if there is no path (kind of obvious if I do say so)
char *path; //And last but not least, a variable used to store the path so it can be used

/**
*This function is for trimming whitespace in a string
*Got it from here https://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way
**/
char *trimwhitespace(char *str){
  char *end;
  while(isspace((unsigned char)*str)) str++;
  if(*str == 0)  // All spaces?
    return str;
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;
  end[1] = '\0';
  return str;
}
/**
*This function is for trimming whitespace in a string
*Got it from here https://stackoverflow.com/questions/3981510/getline-check-if-line-is-whitespace
*I modified this one a bit
**/
int is_empty(const char *s) {
  while (*s != '\0' && *s != '\n' && *s != '\t') {
    if (!isspace((unsigned char)*s)){
      //printf("%d is not whitespace\n", *s);
      return 0;
    }
    s++;
  }
  return 1;
}

int runProcess(char *myargs[]) {
    int rc=fork(); //fork to new process
    wait(NULL); //This guy waits so that output is orderly, took me a while to make use of it
    if(rc<0){  //uh ohh problemmmm
        fprintf(stderr, "%s", "An error has occurred\n");
        exit(1); 
    }
    else if(noPath != 1 && rc == 0){
        if(pathAffected == 0){
            path = strcat(strdup("/bin/"), myargs[0]);
            if(pathAffected == 0 && access(path, X_OK) !=0){//https://stackoverflow.com/questions/230062/whats-the-best-way-to-check-if-a-file-exists-in-c
                path = strcat(strdup("/usr/bin/"), myargs[0]);// tack on arguments
                if(access(path, X_OK) != 0){ fprintf(stderr, "%s", "An error has occurred\n"); exit(0);}
            }             
        }
        else if(numPaths==0 && pathAffected==1){ //only one path added
            path = strcat(path, myargs[0]);
            if(access(path, X_OK) != 0){fprintf(stderr, "%s", "An error has occurred\n"); exit(0);}
        }
        else{
            for(int i = 0; i < numPaths; i++){
                strcat(paths[i], myargs[0]);
                if(access(paths[i], X_OK) == 0){strcpy(path, paths[i]); break;}
            } 
        }
        int returnStatus = execv(path,myargs);
        if(returnStatus == -1){
            fprintf(stderr, "%s", "An error has occurred\n");
            exit(0);
        }        
    }
    return rc;
}

int main(int argc, char* argv[]){
    FILE *inputFile = NULL; //the file used to save user input
    path = (char*) malloc(bufferSize);
    char buffer[bufferSize];
    int looped = 0;

    if(argc == 2){//batch
        char *batchFile = argv[1];//save file name
        inputFile = fopen(batchFile, "r");//open file
        if(inputFile == NULL){ //if there is nothing in file
            fprintf(stderr, "%s", "An error has occurred\n");
            exit(1);
        }
        mode = 1;
    }
    else if(argc == 1){//interactive mode
        inputFile=stdin;
        printf("wish>");
    }
    else{
        fprintf(stderr, "%s", "An error has occurred\n");
        exit(1);
    }
    
    while(fgets(buffer, bufferSize, inputFile)){ //takes user input and saves in buffer
        if(strcmp(buffer, "\n") == 0){continue;} //if user input was only a newline
        else{
            if(strstr(buffer,">")!=NULL){
                int i=0;
                char* args[sizeof(buffer)];
                char* fileName;
                args[0]= strtok(buffer,">");
                if(buffer[0] == '>'){
                    fprintf(stderr, "%s", "An error has occurred\n");
                    exit(0);
                }
                args[0] = trimwhitespace(args[0]);
                while(args[i]!=NULL){
                    i++;
                    args[i]=strtok(NULL,">");
                }
                int numArgsInPath = 0;
                char* argsInPath[sizeof(args[1])];
                char* pathCopy = strdup(args[1]);
                argsInPath[0] = strtok(pathCopy, " ");
                while(argsInPath[numArgsInPath] != NULL){
                    numArgsInPath++;
                    argsInPath[numArgsInPath] = strtok(NULL, " ");
                }
                args[0] = trimwhitespace(args[0]);
                args[1] = trimwhitespace(args[1]);
                if(numArgsInPath > 1){
                    fprintf(stderr, "%s", "An error has occurred\n");
                    exit(0);
                }
                if(strcmp(args[1],"") == 0){
                    fprintf(stderr, "%s", "An error has occurred\n");
                    exit(0);
                }
                if(i>2){
                    fprintf(stderr, "%s", "An error has occurred\n"); 
                    exit(0);    
                }
                else if(i == 1){
                    fprintf(stderr, "%s", "An error has occurred\n"); 
                    exit(0);
                }
                fileName = args[1];
                fflush(stdout);
                (void)close(STDOUT_FILENO);
                open(fileName, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                strcpy(buffer, args[0]);
            }
            int lineEmpty = 0;
            if(is_empty(buffer)){lineEmpty = 1;}
            if(buffer[0] != '\n' && buffer[0] != '\0' && lineEmpty != 1){
                char *insts[sizeof(buffer)]; //String to hold instructions
                insts[0] = strtok(buffer, " \t\n"); //get the first instruction in buffer
                int numArgs = 0;    //keep track of the amount of instructions
                while(insts[numArgs] != NULL){ //count instructions
                    numArgs++;
                    insts[numArgs] = strtok(NULL," \t\n");
                }
                insts[0] = trimwhitespace(insts[0]);
                insts[numArgs + 1] = NULL; //mark end of instructions
                if(strcmp(insts[0], "loop") == 0 && numArgs > 1) {
                    char *loopInsts[sizeof(insts)];
                    int numLoops = atoi(insts[1]);
                    int loopVar = 1;
                    int loopInd = 0;
                    int loopCountNeeded = 0;
                    if(numLoops <= 0){fprintf(stderr, "%s", "An error has occurred\n");}
                    else{
                        for(int i = 2; i < numArgs; i++){
                            loopInsts[i-2] = insts[i];
                        }
                        for(int i = 0; i < numLoops; i++){
                            for(int j = 2; j < numArgs; j++){
                                if(strcmp(insts[j], "$loop") == 0){
                                    loopInd = j-2;
                                    j = numArgs;
                                    loopCountNeeded = 1;
                                }
                            }
                            if(loopCountNeeded){
                                sprintf(loopInsts[loopInd], "%d", loopVar);
                                loopVar++;
                            }
                            runProcess(loopInsts);
                        }
                    }
                    looped = 1;
                }
                else if(strcmp(insts[0],"cd") == 0){//Change Directory
                    if(numArgs == 2){
                        int changed = chdir(insts[1]);
                        if(changed == -1){fprintf(stderr, "%s", "An error has occurred\n");}
                    }
                    else{fprintf(stderr, "%s", "An error has occurred\n");}
                }
                else if(strcmp(insts[0],"path") == 0){
                    pathAffected = 1;
                    if(numArgs == 2){
                        noPath = 0;
                        path = strdup(insts[1]);
                        if(path[strlen(path)-1] != '/'){strcat(path, "/");}    
                    }
                    else if(numArgs == 1){noPath = 1;}
                    else{ 
                        noPath = 0;
                        for(int i = 1; i < numArgs; i++){
                            char *inst1 = strdup(insts[i]);
                            if(inst1[strlen(inst1)-1] != '/'){strcat(inst1, "/");}
                            strcpy(paths[i-1], inst1);
                            numPaths++;
                        }
                    }                    
			    }
                else if(strcmp(insts[0],"exit") == 0) {
                    if(numArgs == 1){exit(0);}
                    else{fprintf(stderr, "%s", "An error has occurred\n");}
                }
                else{
                    if(noPath==1){fprintf(stderr, "%s", "An error has occurred\n");}    
                    else if(!looped){runProcess(insts);}
                    else{fprintf(stderr, "%s", "An error has occurred\n");}          
                }
            }
        }
        if(argc == 1 && mode != 1){printf("wish>");}
    }
}
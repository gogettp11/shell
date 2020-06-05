#include<stdio.h> 
#include<string.h> 
#include<stdlib.h> 
#include<unistd.h> 
#include<sys/types.h> 
#include<sys/wait.h> 
#include <signal.h>

#define BUFSIZE 64
#define DELIM " \t\r\n\a"
#define clear() printf("\033[H\033[J")
#define MAXCOMMANDS 20
#define MAXLINELENGHT 128
FILE * history;
char* current_history[MAXCOMMANDS];
//zmienna pomocnicza ,która sprawia ,że działa wczytywanie z pliku
//bez niej ucina ostatnią linijkę
int xxx = 0;

//wczytuje wejście, funkcja getline wczytuje aż do napotkania znaku entera
char* read_input(){

  char *line = NULL;
  ssize_t bufsize = 0;

  if (getline(&line, &bufsize, stdin) == -1){
    if (feof(stdin)) {
      exit(EXIT_SUCCESS);
    } else  {
      perror("readline error");
      exit(EXIT_FAILURE);
    }
  }
  int lenght = strlen(line);
  line[lenght-1] = 0;
  return (char*)line;
}
//rozbija char zawierający cały input 
//na pojedyncze znaki dzięki którym można uruchomić program i to z argumentami
char **parse(char *line){
  int bufsize = BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;

  if (tokens==NULL) {
    fprintf(stderr, "allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "reallocation error\n");
        exit(EXIT_FAILURE);
      }
    }
    token = strtok(NULL, DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}
//uruchamia program
int execute(char** args){

    pid_t pid, wpid;
    int status;

    pid = fork();

    if (pid == 0) {
        if (execvp(args[0], args) == -1) 
            perror("some error");
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("fork error");
    } else {
        do {
         wpid = waitpid(pid, &status, WUNTRACED);
        }while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}
//wyświetla aktualny folder
void currentDir(){
    char cwd[1024]; 
    getcwd(cwd, sizeof(cwd)); 
    printf("%s", cwd); 
}
//wczytuje jedną linijkę z folderu gdzie zapisana jest historia
int readLine(FILE *file, char* line) {

    int maximumLineLength = MAXLINELENGHT;
    char *lineBuffer = (char *)malloc(sizeof(char) * maximumLineLength);

    if (lineBuffer == NULL) {
        printf("Error allocating memory for line buffer.");
        exit(1);
    }

    char ch = getc(file);
    int count = 0;
    //wczytuje chary aż nie spotka EOF lub \n jeśli okaże się ,że za mało
    //pamięci dla zmiennej zostało przydzielone - realokuje 
    while ((ch != '\n') && (ch != EOF)) {
        if (count == maximumLineLength) {
            maximumLineLength += 128;
            lineBuffer = realloc(lineBuffer, maximumLineLength);
            if (lineBuffer == NULL) {
                printf("Error reallocating space for line buffer.");
                exit(1);
            }
        }
        lineBuffer[count] = ch;
        count++;
        ch = getc(file);
    }

    if(ch == EOF){
      xxx++;
      if(xxx == 2){
        xxx = 0;
        return 1;
      }
    }

    lineBuffer[count] = '\0';
    strncpy(line, lineBuffer, (count));
    free(lineBuffer);
    return 0;
}
//inizjalizuje historię, wczytuje historię, wyświetla powitanie przy włączeniu
void init(){

    clear();
    history = fopen("history.txt", "r");
    if(history == NULL)
      printf("error opening history of commands");
    printf("current user: %s\n", getenv("USER"));
    printf("*********\n");
    printf("current history:\n");
    char *buffer;
    for(int counter = 1; counter < MAXCOMMANDS+1; counter++){
      buffer = (char*)malloc(sizeof(char)*MAXLINELENGHT);
      if(readLine(history, buffer)!=0)
        break;
      current_history[counter-1] = buffer;
      printf("%d: %s\n", counter, current_history[counter-1]);
    }
    if(fclose(history)!=0)
      printf("error closing file");
    sleep(2);
    clear();
}
//zapisuje historię do zmiennej w shellu, przesuwa pozycję o jeden
void write_history(char *line){
  char *buffer[MAXCOMMANDS];
  for(int i = 0; i < MAXCOMMANDS; i++){
      buffer[i] = current_history[i];
  }
  current_history[0] = line;
  for(int i = 0; i < MAXCOMMANDS-1; i++){
    current_history[i+1] =  buffer[i];
  }
}
//zapisuje historię do pliku
void save_history(){
  history = fopen("history.txt","w");
  if(history==NULL)
    printf("error opening file");
  for(int i = 0; i < MAXCOMMANDS; i++)
    fprintf(history,"%s\n",current_history[i]);
  if(fclose(history)!=0)
    printf("error closing file");
}
int main(){
    //zmienna line to cały input
    char *line;
    //args to line rozbite na tokeny
    char **args;
    //status jest jeden jak się wykona program 
    int status;

    init();

  do{
    currentDir();
    printf("> ");
    //wczytuje historię
    line = read_input();
    //zapisuje historię w zmiennej
    write_history(line);
    //zapisuje historię w pliku
    save_history();
    //rozbicie inputu na tokeny
    args = parse(line);
    //wykonanie programu
    status = execute(args);
    free(args);
  }while (status);
}
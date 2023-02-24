#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAXWORDS 10
#define NOT_KCMD -1234567

#define KCMDS_COUNT 15
char kCmds[KCMDS_COUNT][15] = {"kcd", 
                               "kls", 
                               "kpwd",
                               "kcp", 
                               "kexit", 
                               "khistory", 
                               "kstop", 
                               "kcont",
                               "kcat",
                               "khelp",
                               "kclear",
                               "krm",
                               "kmkdir",
                               "krmdir",
                               "kpipe"};

int kcd(char* path) {
    if(chdir(path) != 0) {
        perror("Nu s-a putut schimba directorul curent");
        return 0;
    }
    return 1;
}

int kls() {
    struct dirent** nameList;
    int n = scandir(".", &nameList, NULL, alphasort);
    if(n < 0) {
        perror("Eroare la scandir");
        return 0;
    } else {
        printf("\n");
        while(n--) {
            printf("%s\n", nameList[n]->d_name);
            free(nameList[n]);
        }
        free(nameList);
    }
    return 1;
}

int kpwd() {
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("%s\n", cwd);
    return 1;
}

int kcp(char* src, char* dest) {
    FILE *fsrc, *fdest;
    char c;
    
    if((fsrc = fopen(src, "r")) == NULL) {
        perror("Nu s-a putut deschide fisierul sursa");
        return 0;
    }
    if((fdest = fopen(dest, "w")) == NULL) {
        perror("Nu s-a putut deschide fisierul destinatie");
        return 0;
    }
    c = fgetc(fsrc);
    while(c != EOF) {
        fputc(c, fdest);
        c = fgetc(fsrc);
    }
    return 1;
}

int kcat(char* src) {
    FILE *fsrc;
    char c;

    if((fsrc = fopen(src, "r")) == NULL) {
        perror("Nu s-a putut deschide fisierul sursa");
        return 0;
    }
    c = fgetc(fsrc);
    while(c != EOF) {
        printf("%c", c);
        c = fgetc(fsrc);
    }
    return 1;
}

int khistory() {
    register HIST_ENTRY** historyList = history_list();
    register int i;
    
    printf("Istoric:\n");
    for(i = 0; historyList[i]; i++) {
        printf("%d: %s\n", i+history_base, historyList[i]->line);
    }
    return 1;
}

int krm(char* file) {
    if(remove(file) != 0) {
        perror("Nu s-a putut sterge fisierul");
        return 0;
    }
    return 1;
}

int kmkdir(char* dir) {
    if(mkdir(dir, 0777) != 0) {
        perror("Nu s-a putut crea directorul");
        return 0;
    }
    return 1;
}

int krmkdir(char* dir) {
    if(rmdir(dir) != 0) {
        perror("Nu s-a putut sterge directorul");
        return 0;
    }
    return 1;
}

int kstop(char* pid) {
    if(kill(atoi(pid), SIGSTOP) != 0) {
        perror("Nu s-a putut opri procesul");
        return 0;
    }
    return 1;
}

int kcont(char* pid) {
    if(kill(atoi(pid), SIGCONT) != 0) {
        perror("Nu s-a putut continua procesul");
        return 0;
    }
    return 1;
}

void clear() {
    printf("\033[H\033[J");
}

void init() {
    clear();
    printf("New shell\n");
    printf("Dumitriu Razvan-Cristian, Murica Andrei\n");
    printf("Grupa 252\n\n\n");
    printf("Username: %s\n\n\n", getenv("USER"));
    sleep(2);
    clear();
}

void showCurrentDir() {
    char cwd[1024], hostname[1024];
    getcwd(cwd, sizeof(cwd));
    gethostname(hostname, sizeof(hostname));
    printf("%s@%s: %s$ ", getenv("USER"), hostname, cwd);
}

void khelp() {
    printf("Comenzi disponibile:\n");
    printf("kcd $dir - schimba directorul curent\n");
    printf("kls - afiseaza continutul directorului curent\n");
    printf("kpwd - afiseaza directorul curent\n");
    printf("kcp $src $dest - copiaza continutul fisierului $src in $dest\n");
    printf("kexit - inchide shell-ul\n");
    printf("khistory - afiseaza istoricul comenzilor\n");
    printf("kstop $pid - opreste un proces\n");
    printf("kcont $pid - continua un proces\n");
    printf("kcat $file - afiseaza continutul fisierului $file\n");
    printf("khelp - afiseaza aceasta lista de comenzi\n");
    printf("kclear - sterge ecranul\n");
    printf("krm $file - sterge fisierul $file\n");
    printf("kmkdir $dir - creeaza directorul $dir\n");
    printf("krmdir $dir - sterge directorul $dir\n");
    printf("kpipe $cmd - executa comanda $cmd folosind pipe\n");
    printf("\n");
}

void parsePipeCmds(char** words, char*** cmds, int* cmdCount) {
    int i = 0;
    int j = 0;
    cmds[0] = (char**)malloc(MAXWORDS * sizeof(char*));
    while(words[i] != NULL) {
        if(strcmp(words[i], "|") == 0) {
            cmds[*cmdCount][j] = NULL;
            (*cmdCount)++;
            j = 0;
            cmds[*cmdCount] = (char**)malloc(MAXWORDS * sizeof(char*));
        } else {
            cmds[*cmdCount][j] = words[i];
            j++;
        }
        i++;
    }
    cmds[*cmdCount][j] = NULL;
    (*cmdCount)++;
}

int process(int input, int output, char** cmd) {
    pid_t pid;
    pid = fork();
    if(pid == 0) {
        if(input != STDIN_FILENO) {
            dup2(input, STDIN_FILENO);
            close(input);
        }
        if(output != STDOUT_FILENO) {
            dup2(output, STDOUT_FILENO);
            close(output);
        }
        return execvp(cmd[0], cmd);
    }
    return pid;
}

int kpipe(char*** cmds, int cmdCount) {
    int i;
    int input = STDIN_FILENO, fd[2]; 

    for(i = 0; i < cmdCount - 1; i++) {
        pipe(fd);
        process(input, fd[1], cmds[i]);
        close(fd[1]);
        input = fd[0];
    }
    if(input != STDIN_FILENO) {
        dup2(input, STDIN_FILENO);
    }
    execvp(cmds[i][0], cmds[i]);
    return 1;
}

int readCommand(char* str) {
    char* buf;
    buf = readline("");
    if(strlen(buf) != 0) {
        add_history(buf);
        strcpy(str, buf);
        return 0;
    } else {
        return 1;
    }
}

void parseWords(char* str, char** parsed, int* cnt) {
    for(int i = 0; i < MAXWORDS; i++) {
        parsed[i] = strsep(&str, " ");
        if(parsed[i] == NULL)
            break;
        if(strlen(parsed[i]) == 0) // skip blank words
            i--;
        else
            (*cnt)++;
    }
}

int handleKCmd(char** cmd) {
    int kCmdsCount = KCMDS_COUNT;
    int cmdIndex = -1;

    for(int i = 0; i < kCmdsCount; i++) {
        if(strcmp(cmd[0], kCmds[i]) == 0) {
            cmdIndex = i;
            break;
        }
    }

    switch(cmdIndex) {
        case 0: // kcd
            return kcd(cmd[1]);
        case 1: // kls
            return kls();
        case 2: // kpwd
            return kpwd();
        case 3: // kcp
            return kcp(cmd[1], cmd[2]);
        case 4: // kexit
            exit(0);
        case 5: // khistory
            return khistory();
        case 6: // kstop
            return kstop(cmd[1]);
        case 7: // kcont
            return kcont(cmd[1]);
        case 8: // kcat
            return kcat(cmd[1]);
        case 9: // khelp
            khelp();
            return 1;
        case 10: // kclear
            clear();
            return 1;
        case 11: // krm
            return krm(cmd[1]);
        case 12: // kmkdir
            return kmkdir(cmd[1]);
        case 13: // krmdir
            return krmkdir(cmd[1]);
        case 14: // kpipe
            char input[1024];
            int wordCount = 0;
            printf("kpipe >>> ");
            readCommand(input);
            char** words = (char**)malloc(MAXWORDS * sizeof(char*));
            parseWords(input, words, &wordCount);
            char*** cmds = (char***)malloc(MAXWORDS * sizeof(char**));
            int cmdCount = 0;
            parsePipeCmds(words, cmds, &cmdCount);
            return kpipe(cmds, cmdCount);
        default:
            printf("Comanda necunoscuta, incerc comanda standard...\n");
            break;
    }
    return NOT_KCMD;
}

int handleStdCommand(char** cmd) {
	pid_t pid;
    int status;
	
	pid = fork();

	if(pid == 0) {
		if(execvp(cmd[0], cmd) < 0) {
			perror("Nu s-a putut executa comanda");
			exit(1);		
	
		}
		exit(0);	
	} else if(pid < 0) {
		perror("Nu s-a putut crea procesul");
		exit(1);
    }
	waitpid(pid, &status, 0);
	return WEXITSTATUS(status);
}

int executeCommand(char** cmd) {
    int flag = handleKCmd(cmd);
    if(flag == NOT_KCMD) {
        return !handleStdCommand(cmd);
    }
    return flag;
}

int main() {
    char input[1024];
    char* parsed[1024];
    int i;

    init();

    while(1) {
        showCurrentDir();
        int wordCount = 0;
        
        if(readCommand(input))
            continue;
        parseWords(input, parsed, &wordCount);
        
        bool ok = true;
        char** cmd = (char**)malloc(MAXWORDS * sizeof(char*));
        int cmdWordCount = 0;
        for(i = 0; i < wordCount; i++) {
            if(strcmp(parsed[i], "&&") == 0) {
                cmd[cmdWordCount] = NULL;
                if(ok) {
                    if(!executeCommand(cmd)) {
                        ok = false;
                    }
                    cmdWordCount = 0;
                } else {
                    break;
                }
            } else {
                cmd[cmdWordCount++] = parsed[i];
            }
        }
        if(i == wordCount && ok) {
            cmd[cmdWordCount] = NULL;
            executeCommand(cmd);
        }
        free(cmd);
    }
    return 0;
}
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>





void sigHandler(int sig){
    signal(sig, SIG_DFL);
    char* signame = strsignal(sig);
    printf("the signal name is: %s\n", signame);

    if(sig == SIGINT){
        raise(SIGINT);
    }
    else if(sig == SIGTSTP){
        signal(SIGCONT, sigHandler);
        raise(SIGTSTP);
    }

    else if(sig == SIGCONT){
        signal(SIGTSTP, sigHandler);
        raise(SIGCONT);
    }
}

int main(int argc, char **argv){
    printf("Starting the program\n");
    signal(SIGINT, sigHandler);
    signal(SIGTSTP, sigHandler);
    signal(SIGCONT, sigHandler);
    
	while(1) {
		sleep(2);
	}

	return 0;
}

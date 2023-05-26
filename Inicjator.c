#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <signal.h>
#include <sys/shm.h>
#include <time.h>
#include <string.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

#define SEM1_NAME "sem1"
#define SEM2_NAME "sem2"
#define SEM3_NAME "sem3"
#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)

static int koniec = 0;

int qid, i;
key_t msgkey;
pid_t pid1,pid2,pid3;

struct mymsgbuf {
    long mtype;
    int i;
}queue ;


void zakoncz(){
    printf("!Zakonczono program(SIGQUIT)\n");
    koniec++;
}

void sygnal(int sig){
    printf("!Odebralem sygnal %d\n", sig);
    queue.mtype = 1;
    queue.i = sig;
    msgsnd(qid, &queue, sizeof(struct mymsgbuf) - sizeof(long), 0);
    queue.mtype = 2;
    msgsnd(qid, &queue, sizeof(struct mymsgbuf) - sizeof(long), 0);
    queue.mtype = 3;
    msgsnd(qid, &queue, sizeof(struct mymsgbuf) - sizeof(long), 0);

    kill(pid1, SIGUSR1);
}

void odbierz(int sig){
    sygnal(sig);
}

int main(void){

    char *args1[]={"./P1", NULL};  //powołanie tablic argumentów potrzebnych do uruchomienia programów w procesach potomnych
    char *args2[]={"./P2", NULL};
    char *args3[]={"./P3", NULL};

    int semid, shmid, shmid23;


    sigset_t maskaP;//zdefiniowanie maski sygnalow ktora bedzie przyjmowala tylko te ponizsze
        sigemptyset(&maskaP);
        sigfillset(&maskaP);
        sigdelset(&maskaP,SIGQUIT);//nr 3
        sigdelset(&maskaP,SIGINT);//nr 2
        sigdelset(&maskaP,SIGTRAP);//nr 5
        //sigdelset(&maskaP,14);
        sigprocmask(SIG_SETMASK, &maskaP, NULL);

        signal(SIGQUIT, zakoncz);
        signal(SIGTRAP, odbierz);
        //signal(SIGALRM, odbierz);

        remove("fifo1");
        remove("fifo2");
        remove("fifo3");

    if(mkfifo("fifo1",0666)==-1){
        perror("tworzenie kolejki fifo nie powiodlo sie\n");
    return 0;
    }
    if(mkfifo("fifo2",0666)==-1){
        perror("tworzenie kolejki fifo nie powiodlo sie\n");
    return 0;
    }
    if(mkfifo("fifo3",0666)==-1){
        perror("tworzenie kolejki fifo nie powiodlo sie\n");
    return 0;
    }

    if (sem_unlink(SEM1_NAME) < 0){
        //perror("sem_unlink(3) failed");
        }
    if (sem_unlink(SEM2_NAME) < 0){
        //perror("sem_unlink(3) failed");
        }
    if (sem_unlink(SEM3_NAME) < 0){
        //perror("sem_unlink(3) failed");
        }


    sem_t *sem1 = sem_open(SEM1_NAME, O_CREAT | O_EXCL, SEM_PERMS, 1);
    sem_t *sem2 = sem_open(SEM2_NAME, O_CREAT | O_EXCL, SEM_PERMS, 0);
    sem_t *sem3 = sem_open(SEM3_NAME, O_CREAT | O_EXCL, SEM_PERMS, 0);

    if (sem1 == SEM_FAILED) {
        perror("sem_open(3) 1 error");
        exit(EXIT_FAILURE);
    }
    if (sem_close(sem1) < 0) {
        perror("sem_close(3) failed");
        sem_unlink(SEM1_NAME);
        exit(EXIT_FAILURE);
    }
    if (sem2 == SEM_FAILED) {
        perror("sem_open(3) 2 error");
        exit(EXIT_FAILURE);
    }
    if (sem_close(sem2) < 0) {
        perror("sem_close(3) failed");
        sem_unlink(SEM2_NAME);
        exit(EXIT_FAILURE);
    }
    if (sem3 == SEM_FAILED) {
        perror("sem_open(3) 3 error");
        exit(EXIT_FAILURE);
    }
    if (sem_close(sem3) < 0) {
        perror("sem_close(3) failed");
        sem_unlink(SEM3_NAME);
        exit(EXIT_FAILURE);
    }

    shmid = shmget(45286, sizeof(char), IPC_CREAT|0600);  //utworzenie segmentu pamięci współdzielonej z pojemnością na chary
    if(shmid == -1){
        perror("Błąd tworzenia segmentu pamieci wspoldzielonej Init");
        exit(1);
    }
    shmid23 = shmget(45287, sizeof(int), IPC_CREAT|0600);  //utworzenie segmentu pamięci współdzielonej z pojemnością na chary
    if(shmid == -1){
        perror("Błąd tworzenia segmentu pamieci wspoldzielonej Init");
        exit(1);
    }

    msgkey = ftok(".", 'm');
    if((qid = msgget(msgkey, IPC_CREAT | 0660)) == -1) {  //utworzenie kolejki komunikatów
        perror("Blad tworzenia kolejki komunikatow Init\n");
        exit(1);
    }

    if(fork() == 0){  //powołanie 1. procesu potomnego
        pid1 = getpid();
        printf("P1 pid: %d\n\n", pid1);
        if(execvp(args1[0],args1) == -1){
            perror("Błąd otwarcia P1");
            exit(1);
        }
    }
    else if(fork() == 0){  //powołanie 2. procesu potomnego
        pid2 = getpid();
        printf("P2 pid: %d\n", pid2);
	if(execvp(args2[0],args2) == -1){
            perror("Błąd otwarcia P2");
            exit(1);
        }
    }
    else if(fork() == 0){  //powołanie 3. procesu potomnego
        pid3 = getpid();
        printf("P3 pid: %d\n", pid3);
        if(execvp(args3[0],args3) == -1){
            perror("Błąd otwarcia P3");
            exit(1);
        }
    }
    else{

    while(koniec == 0){ }
        remove("fifo1");
        remove("fifo2");
        remove("fifo3");
        shmctl(shmid, IPC_RMID, NULL);//rmid sluzy do usuwania share_memory
        shmctl(shmid23, IPC_RMID, NULL);  //usunięcie segmentu pamięci wspóldzielonej
        if( msgctl(qid, IPC_RMID, 0) == -1) {   //usunięcie kolejki komunikatów z pamięci
        perror("Blad usuwania kolejki komunikatow");
        exit(1);
        }
        if (sem_unlink(SEM1_NAME) < 0){
        perror("sem_unlink(3) failed");}
        if (sem_unlink(SEM2_NAME) < 0){
        perror("sem_unlink(3) failed");}
        if (sem_unlink(SEM3_NAME) < 0){
        perror("sem_unlink(3) failed");}
        }
        //kill(pid1, SIGKILL);
        //kill(pid2, SIGKILL);
        //kill(pid3, SIGKILL);


    printf("[Koniec]\n");
}

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
#include <sys/ipc.h>

#define SEM3_NAME "sem3"
#define SEM1_NAME "sem1"
#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)

static int koniec = 0;
static int rob = 1;
sem_t *semafor3;
sem_t *sem3;
sem_t *sem1;
int qid, i;
key_t msgkey;


struct mymsgbuf {
    long mtype;
    char i;
}queue ;

struct mymsgbuf readbuffer;


void end(){
    printf("P3: KONIEC\n");
    koniec=1;
    rob=0;
    if (sem_post(semafor3) < 0) {
        perror("sem_post error on P3");
    }
    kill(getppid(),SIGQUIT);
}

void sygnal(int sig){
    printf("\nP3: Odebralem sygnal %d\n", sig);
    if(sig == 5){
    kill(getppid(), SIGTRAP);}
}

void kolejka(){
    sem_wait(semafor3);
    printf("\nP3: Odebralem sygnal o odczytaniu kolejki\n");
    readbuffer.mtype = 3;
    msgrcv(qid, &readbuffer, sizeof(struct mymsgbuf) - sizeof(long), readbuffer.mtype, IPC_NOWAIT);
    printf("P3: Odczytalem: %d\n", readbuffer.i);
}

void zakoncz(){
    int i,j;
    char b[2]="1";
    kill(getpid()-1,SIGINT);
    kill(getpid()-2,SIGINT);
    i=open("fifo1",O_WRONLY);
    write(i,b,2);
    close(i);
    j=open("fifo2",O_WRONLY);
    write(j,b,2);
    close(j);
    end();
}

void odbierz(int sig){
    sygnal(sig);
}


void odczytaj(){
    kolejka();
}



int main(void){

    int semid, shmid23;
    char* buf23;


    if (sem_unlink("semafor3") < 0){
        //perror("sem_unlink(3) failed");
        }

    semafor3 = sem_open("semafor3", O_CREAT | O_EXCL, SEM_PERMS, 1);
    if (semafor3 == SEM_FAILED) {
        perror("sem_open(3) 1 error P3");
        exit(EXIT_FAILURE);
    }

    sigset_t maskaP;
    sigemptyset(&maskaP);
    sigfillset(&maskaP);
    sigdelset(&maskaP,SIGQUIT);//nr 3
    sigdelset(&maskaP,SIGTRAP);//nr 5
    //sigdelset(&maskaP,14);
    sigdelset(&maskaP,SIGINT);//nr 2
    sigdelset(&maskaP,SIGUSR1);//nr 10
    sigprocmask(SIG_SETMASK, &maskaP, NULL);

    signal(SIGQUIT,zakoncz);
    signal(SIGTRAP,odbierz);
    //signal(SIGALRM,odbierz);
    signal(SIGUSR1,odczytaj);

    sem3 = sem_open(SEM3_NAME, O_RDWR);
    if (sem3 == SEM_FAILED) {
        perror("sem_open(3) 3 failed P3");
        exit(EXIT_FAILURE);
    }
    sem1 = sem_open(SEM1_NAME, O_RDWR);
    if (sem1 == SEM_FAILED) {
        perror("sem_open(3) 1 failed P3");
        exit(EXIT_FAILURE);
    }

    shmid23 = shmget(45287, sizeof(int), 0600);  //otwarcie segmentu pamięci wspol dla zamienionej danej
    if(shmid23 == -1){
        perror("Błąd przyłączenia segmentu pamieci wspoldzielonej P3");
        exit(1);
    }

    buf23 = shmat(shmid23, NULL, 0);
    if(buf23 == NULL){
        perror("Błąd przyłączania segmentu pamięci współdzielonej P3");
    }

    msgkey = ftok(".", 'm');
    if((qid = msgget(msgkey, 0660 | IPC_CREAT)) == -1) {  //utworzenie kolejki komunikatów
        perror("Blad otwierania kolejki komunikatow P3\n");
        exit(1);
    }

    while(koniec == 0){
        if(rob == 1){
		    if (sem_wait(sem3) < 0) {
		        perror("sem_wait(3) failed on P3");
		        continue;
		    }
		    if(sem_wait(semafor3) < 0){
		        perror("sem_wait failed on P3");
		        continue;
		    }
		    if(koniec == 0){
				printf("P3 - odczytałem: %s", buf23);

				printf("\n\n");
				
				if (sem_post(semafor3) < 0) {
				    perror("sem_post error on P3");
				}
				if (sem_post(sem1) < 0) {
				    perror("sem_post(3) error on P3");
				}
		    }
        }
    }

    if (sem_close(semafor3) < 0){
        perror("sem_close(3) sem failed P3");}
    if (sem_close(sem3) < 0){
        perror("sem_close(3) 3 failed P3");}
    if (sem_close(sem1) < 0){
        perror("sem_close(3) 1 failed P3");}
    if (sem_unlink("semafor3") < 0){
        perror("sem_unlink(3) failed");}

return 0;
}

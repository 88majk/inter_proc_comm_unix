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
#define SEM2_NAME "sem2"
#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)


static int koniec = 0;
static int rob = 1;

sem_t *semafor2;

int qid, i;
key_t msgkey;

struct mymsgbuf {
    long mtype;
    int i;
}queue ;

struct mymsgbuf readbuffer;

void end(){
    printf("P2: KONIEC\n");
    koniec=1;
    rob = 0;
    if (sem_post(semafor2) < 0) {
        perror("sem_post error on P2");
    }
    kill(getppid(),SIGQUIT);
}

void kolejka(){
    sem_wait(semafor2);
    printf("P2: Odebralem sygnal o odczytaniu kolejki\n");
    readbuffer.mtype = 2;
    msgrcv(qid, &readbuffer, sizeof(struct mymsgbuf) - sizeof(long), readbuffer.mtype, IPC_NOWAIT);//odebranie komunikatu
    printf("P2: Odczytalem: %d\n", readbuffer.i);
    kill(getpid()+1, SIGUSR1);
}

void zakoncz(){
    int i,j;
    char b[2]="1";
    kill(getpid()-1,SIGINT);
    kill(getpid()+1,SIGINT);
    i=open("fifo1",O_WRONLY);
    write(i,b,2);
    close(i);
    j=open("fifo3",O_WRONLY);
    write(j,b,2);
    close(j);
    end();
}


void odczytaj(){
    kolejka();
}


int main(void){

    int semid, shmid, shmid23;
    char* buf;
    char* buf23;

    if (sem_unlink("semafor2") < 0){
        //perror("sem_unlink(3) failed");
        }

    semafor2 = sem_open("semafor2", O_CREAT | O_EXCL, SEM_PERMS, 1);
    if (semafor2 == SEM_FAILED) {
        perror("sem_open(3) 1 error P2");
        exit(EXIT_FAILURE);
    }

    sigset_t maskaP;
        sigemptyset(&maskaP);
        sigfillset(&maskaP);
        sigdelset(&maskaP,SIGQUIT);//nr 5
        sigdelset(&maskaP,SIGINT);//nr 2
        sigdelset(&maskaP,SIGUSR1);//nr 10
        sigprocmask(SIG_SETMASK, &maskaP, NULL);

    signal(SIGQUIT,zakoncz);
    signal(SIGUSR1,odczytaj);

    sem_t *sem3 = sem_open(SEM3_NAME, O_RDWR);
    if (sem3 == SEM_FAILED) {
        perror("sem_open(3) 3 failed P2");
        exit(EXIT_FAILURE);
    }
    sem_t *sem2 = sem_open(SEM2_NAME, O_RDWR);
    if (sem2 == SEM_FAILED) {
        perror("sem_open(3) 2 failed P2");
        exit(EXIT_FAILURE);
    }

    shmid = shmget(45286, sizeof(char), 0600);  //otwarcie segmentu pamięci współdzielonej dla odebranej danej
    if(shmid == -1){
        perror("Błąd przyłączenia segmentu pamieci wspoldzielonej P2");
        exit(1);
    }

    shmid23 = shmget(45287, sizeof(int), 0600);  //otwarcie segmentu pamięci współdzielonej dla zamienionej danej
    if(shmid == -1){
        perror("Błąd przyłączenia segmentu pamieci wspoldzielonej P2");
        exit(1);
    }

    buf = shmat(shmid, NULL, 0);
    if(buf == NULL){
        perror("Błąd przyłączania segmentu pamięci współdzielonej P2");
    }
    buf23 = shmat(shmid23, NULL, 0);
    if(buf23 == NULL){
        perror("Błąd przyłączania segmentu pamięci współdzielonej P2");
    }

    msgkey = ftok(".", 'm');
    if((qid = msgget(msgkey, 0660 | IPC_CREAT)) == -1) {  //otwarcie kolejki komunikatów
        perror("Blad otwierania kolejki komunikatow P3\n");
        exit(1);
    }

    while(koniec == 0){
        if(rob == 1){
		    if (sem_wait(sem2) < 0) {
		        perror("sem_wait(3) failed on P2");
		        continue;
		    }
		    if(sem_wait(semafor2) < 0){
		        perror("sem_wait failed on P2");
		        continue;
		    }
		    if(koniec == 0){
				printf("P2 - Odebrałem z pamięci: %s", buf);
				int len = strlen(buf);
				int znaki = 0;
				int loop = 0;
				while(buf[loop] != '\0'){
				    znaki++;
				    loop++;
				}
				printf("\nZliczam znaki...\n");
				sprintf(buf23, "%d", znaki);

				printf("P2 - wysyłam: %s", buf23);
		    }

		    printf("\n");

		    if (sem_post(semafor2) < 0) {
		        perror("sem_post error on P2");
		    }
		    if (sem_post(sem3) < 0) {
		        perror("sem_post(3) error on P2");
		    }
        }
      }//end while  dla koniec



    if (sem_close(semafor2) < 0){
        perror("sem_close(3) sem failed P2");}
    if (sem_close(sem3) < 0){
        perror("sem_close(3) 3 failed P2");}
    if (sem_close(sem2) < 0){
        perror("sem_close(3) 2 failed P2");}
    if (sem_unlink("semafor2") < 0){
        perror("sem_unlink(3) failed");}

    shmdt(buf);

return 0;
}

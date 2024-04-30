#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>

pid_t m;
int* tablica_zmiennych;
char *shared_memory;

//Funkcje obsługujące sygnały
//Proces musi zalarmować pozostałe procesy o otrzymaniu sygnału

void sigrtmin_handler3(int sig)
{
    
    kill(m, SIGRTMIN);
    signal(sig, SIG_IGN);
}

void sigpwr_handler3(int sig)
{
    signal(sig, SIG_IGN);
    kill(m,SIGPWR); 

    signal(sig, SIG_IGN);
}

void sigsys_handler3(int sig)
{

    kill(m,SIGSYS);    
    signal(sig, SIG_IGN);
}

//Funkcja wątku wyłapującego sygnały
void* sygnalThread3(void* args)
{
    while(1)
        {
            signal(SIGSYS,sigsys_handler3);
            signal(SIGPWR,sigpwr_handler3);
            signal(SIGRTMIN, sigrtmin_handler3);        
        }

        return NULL;
}


void cleanUP3(char *shared_memory,int *tablica_zmiennych)
{
	if (shmdt(shared_memory) == -1) {
	    perror("shmdt");
	}
    if (shmdt(tablica_zmiennych) == -1) {
	    perror("shmdt");
	}
}

void sigio_handler3(int sig)
{
    signal(sig, SIG_IGN);
    shmdt(shared_memory);
    shmdt(tablica_zmiennych);
    kill(m, SIGIO);
    sleep(2);
    exit(1);
}


//Tworzenie struktury semafora razem z jego funkcjami do podnoszenia i opuszczania wartości
static struct sembuf buf;
void podnies(int semid, int semnum)
{
    buf.sem_num = semnum;
    buf.sem_op = 1;
    buf.sem_flg = SEM_UNDO;
    if (semop(semid, &buf, 1) == -1)
    {
        perror("Podnoszenie semafora");
        exit(1);
    }
}

void opusc(int semid, int semnum)
{
    buf.sem_num = semnum;
    buf.sem_op = -1;
    buf.sem_flg = 0;
    while (semop(semid, &buf, 1) == -1) {
            if (errno != EINTR) {
                perror("Opuszczenie semafora3");
                exit(1);
            }
    }
}

int main()
{
    m = getppid();
    int semid;
    int shmid_pidy, shmid_dla_procesu,shmid_dla_sygnalow;


    signal(SIGIO, sigio_handler3);
    pthread_t threadId3;
    if (pthread_create(&threadId3, NULL, sygnalThread3, NULL) != 0) {
        perror("pthread_create");
        exit(1);
    }

///////////////////////////////////////////////////////////////////////////////////
// Otwarcie tablicy semaforów
    semid = semget(200, 6, IPC_CREAT | 0600);
    if (semid == -1)
    {
        perror("Otwarcie tablicy semaforow");
        exit(1);
    }


///////////////////////////////////////////////////////////////////////////////////
// Dołączanie do pamięci wspóldzielonej dla procesów 2-3
    shmid_dla_procesu = shmget(10, sizeof(pid_t)*4, IPC_CREAT | 0600);
    if (shmid_dla_procesu == -1)
    {
        perror("Uzyskanie dostepu do segmentu pamieci wspoldzielonej");
        exit(1);
    }

    shared_memory = shmat(shmid_dla_procesu, NULL, 0);
    if (shared_memory == (char *) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

///////////////////////////////////////////////////////////////////////////////////
// Dołączanie do pamięci współdzielonej sygnałów
    shmid_dla_sygnalow = shmget(999, sizeof(int)*10, IPC_CREAT | 0600);
    if (shmid_dla_sygnalow == -1)
    {
        perror("Utworzenie segmentu pamieci wspoldzielonej");
        exit(1);
    }
    tablica_zmiennych = shmat(shmid_dla_sygnalow, NULL, 0);
    if (tablica_zmiennych == NULL)
    {
        perror("Przylaczenie segmentu pamieci wspoldzielonej");
        exit(1);
    }  

///////////////////////////////////////////////////////////////////////////////////
  

    opusc(semid,3);

    char zakoncz[] = "Koniec";
    while(1)
    {
        opusc(semid,5);

        while(tablica_zmiennych[1] == 0){
        }

        if(strcmp(zakoncz,shared_memory) == 0)
            break;
        printf("Odczytano z pamięci współdzielonej: %s\n",shared_memory);
        podnies(semid,4);       
    }

    sleep(1);

    cleanUP3(shared_memory,tablica_zmiennych);

    return 0;
}

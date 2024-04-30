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

//tablica_zmiennych[1] - blokuje przekaz i doczyt procesow jesli rowne 0
//tablica_zmiennych[2] - blokuje kodowanie jesli rowne 0


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                          //
//      S1  ->  Zakonczenie działania - zwolnienie wszytskich zasobow uzywanych przez procesy                           -  29) SIGIO        //
//      S2  ->  Wstrzymanie działania - Zatrzymanie wymiany danych przez procesy                                        -  30) SIGPWR       //
//      S3  ->  Wznownienie działania - Wznowienie wymiany danych przez procesy                                         -  31) SIGSYS       //
//      S4  ->  Właczenie/Wyłączenie kodowania - czyli dane miedzy procesem 2-3 dane moga zostac wyslane niezmienione   -  34) SIGRTMIN     //
//                                                                                                                                          //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

pid_t  m,x,y,z;
int *tablica_zmiennych;
DIR *dir;
FILE *file1;
FILE *file2;

void cleanUP1(DIR *dir,FILE *file)
{
    if (dir != NULL) {
        if (closedir(dir) == -1) {
            perror("closedir");
        }
    }
    if (file != NULL) {
            if (fclose(file) == -1) {
                perror("fclose");
            }
    }
}

void cleanUP2(FILE *file)
{

    if (file != NULL) {
            if (fclose(file) == -1) {
                perror("fclose");
            }
    }
}


void sigio_handler1(int sig)
{
    signal(sig, SIG_IGN);
    cleanUP1(dir,file1);
    kill(m, SIGIO);
    exit(1);
}

void sigio_handler2(int sig)
{
    signal(sig, SIG_IGN);
    cleanUP2(file2);
    kill(m, SIGIO);
    exit(1);
}

void sigio_handlerM(int sig)
{
    signal(sig, SIG_IGN);
    kill(x,SIGIO);
    kill(y,SIGIO);
    kill(z,SIGIO);
}

void sigrtmin_handlerM(int sig)
{
    signal(sig, SIG_IGN);
    if(tablica_zmiennych[2] == 1)
        tablica_zmiennych[2] = 0;
    else
        tablica_zmiennych[2] = 1;
}

void sigpwr_handlerM(int sig)
{
    tablica_zmiennych[1] = 0;
}

void sigsys_handlerM(int sig)
{
    signal(sig, SIG_IGN);
    tablica_zmiennych[1] = 1;
}

//Funkcja wykonywana przez wątek którego celem jest wyłapanie sygnałów
void* sygnalThreadM(void* args)
{
    while(1)
    {
        signal(SIGSYS,sigsys_handlerM);
        signal(SIGPWR,sigpwr_handlerM);
        signal(SIGRTMIN, sigrtmin_handlerM);        
    }

    return NULL;
}


void sigrtmin_handler2(int sig)
{
    signal(sig, SIG_IGN);
    kill(m, SIGRTMIN);

}

void sigpwr_handler2(int sig)
{
    signal(sig, SIG_IGN);
    kill(m,SIGPWR);
}

void sigsys_handler2(int sig)
{
    signal(sig, SIG_IGN);
    kill(m,SIGSYS);
}

void* sygnalThread2(void* args)
{

    while(1)
    {
        signal(SIGSYS,sigsys_handler2);
        signal(SIGPWR,sigpwr_handler2);
        signal(SIGRTMIN, sigrtmin_handler2);        
    }

    return NULL;
}




void sigrtmin_handler1(int sig)
{
    signal(sig, SIG_IGN);
    kill(m, SIGRTMIN);

}

void sigpwr_handler1(int sig)
{
    
    signal(sig, SIG_IGN);
    kill(m,SIGPWR);
}

void sigsys_handler1(int sig)
{
    signal(sig, SIG_IGN);
    kill(m,SIGSYS);
}

void* sygnalThread1(void* args)
{

    while(1)
    {
        signal(SIGSYS,sigsys_handler1);
        signal(SIGPWR,sigpwr_handler1);
        signal(SIGRTMIN, sigrtmin_handler1);        
    }

    return NULL;
}




static struct sembuf buf;
void podnies(int semid, int semnum)
{
    buf.sem_num = semnum;
    buf.sem_op = 1;
    buf.sem_flg = 0;

    while (semop(semid, &buf, 1) == -1) {
        if (errno != EINTR) {
            perror("Podnoszenie semafora");
            exit(1);
        }
    }
}

void opusc(int semid, int semnum)
{
    buf.sem_num = semnum;
    buf.sem_op = -1;
    buf.sem_flg = 0;
    while (semop(semid, &buf, 1) == -1) {
            if (errno != EINTR) {
                perror("Opuszczenie semafora1");
                exit(1);
            }
    }
}

//Funkcja Kodujaca na postać HAX
char* convertToHex(const char *path) {
    int i = 0;
    int size = 0;

    // Liczymy długość ciągu
    while (path[size] != '\0') {
        size++;
    }

    // Tworzymy bufor dla zakodowanego ciągu (dwa znaki na jeden znak oryginalny)
    char* code = malloc(sizeof(char) * (size * 2 + 1));

    // Zakodowanie ciągu do tymczasowego bufora
    char tempBuffer[size * 2 + 1];
    for (i = 0; i < size; i++) {
        sprintf(&tempBuffer[i * 2], "%02X", (unsigned char)path[i]);
    }

    // Kopiujemy zakodowany ciąg do wynikowego bufora (bez dodatkowych nulli)
    strncpy(code, tempBuffer, size * 2);

    // Dodajemy null na końcu zakodowanego ciągu
    code[size * 2] = '\0';

    return code;
}
void saveDirectoryContentsToFile(DIR *dir, const char *directoryPath, const char *outputFilePath) {
    FILE *outputFile = fopen(outputFilePath, "w");
    if (outputFile == NULL) {
        perror("Nie można otworzyć pliku do zapisu");
        closedir(dir);
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Pomijamy specjalne wpisy dotyczące katalogu (. i ..)
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            fprintf(outputFile, "%s/%s\n", directoryPath, entry->d_name);
        }
    }

    fclose(outputFile);
    closedir(dir);
}
void funkcja_procesu_pierwszego(FILE *file, DIR *dir,int pipe_fd[2])
{
    close(pipe_fd[0]);                   // Zamykamy odczyt, nie będzie go używał

        char path[100];
        dir = NULL;

        while (dir == NULL)
        {
            printf("Podaj sciezke: ");
            scanf("%s",path);

            dir = opendir(path);

            if (dir) {
                printf("Folder istnieje. Kontynuuje...\n\n");
            } else {
                printf("Folder nie istnieje, sprobuj ponownie.\n");
            }           
        }

        const char *outputFilePath = "lista_plikow.txt"; // Nazwa pliku wyjsciowego
        saveDirectoryContentsToFile(dir, path, outputFilePath);

        // Na tym poziomie W folderze /home/student jest utworzony plik lista_plikow.txt z wszytskimi zasobami z wybranej sciezki

        // Otwórz plik i przekieruj jego zawartość do potoku
        file = fopen("lista_plikow.txt", "r");
        if (file == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        char buffer1[3000];
        size_t bytesRead1;

        while(tablica_zmiennych[1] == 0){
            sleep(1);
        }

        while ((bytesRead1 = fread(buffer1, 1, sizeof(buffer1), file)) > 0) {
            write(pipe_fd[1], buffer1, bytesRead1);
        }

        fclose(file);
        close(pipe_fd[1]);

        sleep(1);

}
void funkcja_procesu_drugiego(FILE *file, int pipe_fd[2],char *shared_memory,int semid)
{
    int i=0;        
    int j=0;
    close(pipe_fd[1]);  // Zamknięcie zapisu, nie będzie go używał
    // Przykład: Dynamicznie alokuj pamięć na bufor i czytaj dane z potoku
        printf("Pusto");
    int ilosc_znakow = 0;
    char licznik;
    file = fopen("lista_plikow.txt", "r");
    if (file == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    while ((licznik = fgetc(file)) != EOF) 
            ilosc_znakow++;     
    if (ilosc_znakow == 0)
        printf("Pusto");
    char litera;
    char* slowo = malloc(sizeof(char)*50);
    char *zakodowane_slowo;
    int size;

    fclose(file);
    char *buffer2 = malloc(sizeof(char) * ilosc_znakow + 1); // Alokacja pamięci dla bufora

    while(tablica_zmiennych[1] == 0){
        sleep(1);
    }

    read(pipe_fd[0], buffer2, ilosc_znakow+1);

    // Dodaj znak null na koniec bufora
    buffer2[ilosc_znakow] = '\0';
    podnies(semid,3);
    while((litera = buffer2[j]) != '\0')
    {
        j++;
        if (litera == '\n')
        {
            opusc(semid,4);
            slowo[i] = '\0';
            size = i;
            i = 0;    

            while(tablica_zmiennych[1] == 0){
                sleep(1);
            }
            if(tablica_zmiennych[2] == 0)
            {
                strcpy(shared_memory,slowo);
                zakodowane_slowo = convertToHex(slowo);
                printf("%s -:- %s\n",slowo,zakodowane_slowo);
            }
            else
            {
                zakodowane_slowo = convertToHex(slowo);
                printf("%s -:- %s\n",slowo,zakodowane_slowo);
                strcpy(shared_memory,zakodowane_slowo);
            }
            sleep(1);
            podnies(semid,5);
        }
        else 
        {
            slowo[i] = litera;
            i++;
        }
    }
    char zakoncz[] = "Koniec";
    strcpy(shared_memory,zakoncz); 
}

int main()
{
    int semid;    
    int shmid_dla_procesu, shmid_dla_sygnalow, i;
    m = getpid();
///////////////////////////////////////////////////////////////////////////////////
//Tworzenie wątku wyłapującego sygnały
    pthread_t threadId;
    if (pthread_create(&threadId, NULL, sygnalThreadM, NULL) != 0) {
        perror("pthread_create");
        exit(1);
    }


///////////////////////////////////////////////////////////////////////////////////
//Tworzenie i inicjalizowanie wartościami semafory
  
    semid = semget(200, 6, IPC_CREAT | 0600);
    if (semid == -1)
    {
        perror("Utworzenie tablicy semaforow1");
        exit(1);
    }
    if (semctl(semid, 0, SETVAL, (int)1) == -1)
    {
        perror("Nadanie wartosci semaforowi producenta");
        exit(1);
    }
    if (semctl(semid, 1, SETVAL, (int)0) == -1)
    {
        perror("Nadanie wartosci semaforowi konsumenta 1");
        exit(1);
    }
    if (semctl(semid, 2, SETVAL, (int)0) == -1)
    {
        perror("Nadanie wartosci semaforowi konsumenta 2");
        exit(1);
    }
    if (semctl(semid, 3, SETVAL, (int)0) == -1)
    {
        perror("Nadanie wartosci semaforowi konsumenta 2");
        exit(1);
    }
    if (semctl(semid, 4, SETVAL, (int)1) == -1)
    {
        perror("Nadanie wartosci semaforowi konsumenta 2");
        exit(1);
    }
    if (semctl(semid, 5, SETVAL, (int)0) == -1)
    {
        perror("Nadanie wartosci semaforowi konsumenta 2");
        exit(1);
    }  
///////////////////////////////////////////////////////////////////////////////////
// Tworzenie pamięci współdzielonej dla zmiennych wpływających na zachowowanie programu
  
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
// Tworzenie pamięci współdzielonej dla procesów 2-3

    shmid_dla_procesu = shmget(10, 3000, IPC_CREAT | 0666);
    if (shmid_dla_procesu == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    char *shared_memory = shmat(shmid_dla_procesu, NULL, 0);
    if (shared_memory == (char *) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
///////////////////////////////////////////////////////////////////////////////////
// Tworzenie pipe dla procesów 1-2
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) 
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
///////////////////////////////////////////////////////////////////////////////////

//  PROCES 1 
//  Wczytuje sciezke od uzytkownika
//  Przesyla wszytskie zasoby z danego katalogu do Procesu 2
    if ((x = fork()) == 0) 
    {
        signal(SIGIO, sigio_handler1);
        pthread_t threadId1;
      
      //Tworzenie wątku wyłapującego sygnały
        if (pthread_create(&threadId1, NULL, sygnalThread1, NULL) != 0) {
            perror("pthread_create");
            exit(1);
        }

        semid = semget(200, 6, IPC_CREAT | 0600);
        if (semid == -1)
        {
            perror("Otwarcie tablicy semaforow");
            exit(1);
        }
        opusc(semid, 1);
        sleep(1);
        funkcja_procesu_pierwszego(file1,dir,pipe_fd);
        podnies(semid,2);

        while(1)
            ;
        

        exit(EXIT_SUCCESS);
    }

//  PROCES 2    (PIPE)
//  Pobiera i koduje pobrane dane
//  Wypisuje na ekranie sciezka_orginalna -:- sciezka_zakodowana
//  Wysyla zakodowane dane do Procesu 3
    else if ((y = fork()) == 0)
    {
        signal(SIGIO,sigio_handler2);
        pthread_t threadId2;
        if (pthread_create(&threadId2, NULL, sygnalThread2, NULL) != 0) {
            perror("pthread_create");
            exit(1);
        }
        //signal(SIGIO, koniec_handler_P);
        semid = semget(200, 6, IPC_CREAT | 0600);
        if (semid == -1)
        {
            perror("Otwarcie tablicy semaforow");
            exit(1);
        }
            opusc(semid, 2);
            funkcja_procesu_drugiego(file2,pipe_fd,shared_memory,semid);    
            sleep(1);


        exit(EXIT_SUCCESS);
    }

//  PROCES 3    (PAMIEC WSPOLDZIELONA)
//  Wypisuje pobrane dane z Procesu 2 (zakodowane sciezki)
    else if ((z = fork()) == 0)
    {
        execl("./proces3", "proces3", NULL);
        perror("Uruchomienie programu proces3");
    }

//  Proces macierzysty czeka i wznawia prace gdy konczy sie program
//  Ma posprzatac przed zamknieciem
    else
    {
        signal(SIGIO,sigio_handlerM);
        opusc(semid,0);
        tablica_zmiennych[2] = 1;
        tablica_zmiennych[1] = 1;

        printf("Proces M: PID %6d\n",m);
        printf("Proces 1: PID %6d\n",x);
        printf("Proces 2: PID %6d\n",y);
        printf("Proces 3: PID %6d\n\n",z);
        podnies(semid, 1);

        pid_t result2,result3;
        int status2,status3;

      do {
	    result3 = waitpid(z, &status3, WNOHANG);
        sleep(1);
    
	} while (result3 == 0);

    do {
	    result2 = waitpid(y, &status2, WNOHANG);
        sleep(1);
    
	} while (result2 == 0);
        kill(x,SIGINT);
        sleep(2);        
        printf("\n\n(M)Wszytskie procesy potomne sie skoczyly, zamykam sie\n");

        shmdt(shared_memory);
        shmdt(tablica_zmiennych);
        shmctl(shmid_dla_procesu, IPC_RMID, NULL);
        semctl(semid, 0, IPC_RMID);



        return 0;
    }



}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#define NB_CLIENTS_TOTAL 10
int sem_id;
int pipe_fd[2];

typedef struct {
    int client_id;
    int license_number;
    time_t timestamp;  // Temps d'attribution de la licence
} LicenseRequest;

void P(int sem_id) {
    struct sembuf operation;
    operation.sem_num = 0;
    operation.sem_op = -1;
    operation.sem_flg = 0;
    semop(sem_id, &operation, 1);
}

void V(int sem_id) {
    struct sembuf operation;
    operation.sem_num = 0;
    operation.sem_op = 1;
    operation.sem_flg = 0;
    semop(sem_id, &operation, 1);
}

void client(int id) {
    LicenseRequest request;

    // Demande de licence au serveur
    P(sem_id);
    printf("Client %d : Demande de licence\n", id);

    // Lecture de la demande du serveur depuis le pipe
    if (read(pipe_fd[0], &request, sizeof(LicenseRequest)) > 0) {
        // Affichage de la demande du serveur
        printf("Client %d : Requête du serveur - Licence %d\n", id, request.license_number);
        // Vous pouvez implémenter la communication spécifique avec le client pour l'interroger sur son utilisation ici
    }

    // Libération de la licence
    P(sem_id);
    printf("Client %d : Libération de la licence\n", id);
    V(sem_id);

    exit(0);
}

int main() {
    // Création des sémaphores
    sem_id = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT);
    semctl(sem_id, 0, SETVAL, 1);

    // Création du pipe
    if (pipe(pipe_fd) == -1) {
        perror("Erreur lors de la création du pipe");
        exit(EXIT_FAILURE);
    }

    // Fermeture de l'extrémité de lecture du pipe dans le processus principal
    close(pipe_fd[0]);

    // Création des processus clients
    for (int i = 0; i < NB_CLIENTS_TOTAL; ++i) {
        if (fork() == 0) {
            client(i);
        }
    }

    // Fermeture de l'extrémité d'écriture du pipe dans le processus principal
    close(pipe_fd[1]);

    // Attente de la fin des processus clients
    for (int i = 0; i < NB_CLIENTS_TOTAL; ++i) {
        wait(NULL);
    }

    // Suppression des sémaphores
    semctl(sem_id, 0, IPC_RMID);

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <time.h>

#define NB_LICENCES 5
#define TIME_LIMIT 10  // Temps limite d'utilisation en secondes
#define CHECK_INTERVAL 3  // Intervalle de vérification en secondes

int sem_id;
int pipe_fd[2];

typedef struct {
    int client_id;
    int license_number;
    time_t timestamp;  // Temps d'attribution de la licence
} LicenseRequest;

LicenseRequest* license_queue;
int queue_size = 0;
int queue_capacity = 10;  // Ajustez selon vos besoins

int licenses_available = NB_LICENCES;
int next_license_number = 1;

void enqueue_request(int client_id) {
    if (queue_size < queue_capacity) {
        LicenseRequest request;
        request.client_id = client_id;
        request.license_number = next_license_number++;
        request.timestamp = time(NULL);
        license_queue[queue_size++] = request;
    }
}

void dequeue_request() {
    if (queue_size > 0) {
        // Priorité au dernier client
        int max_timestamp_index = 0;
        for (int i = 1; i < queue_size; ++i) {
            if (license_queue[i].timestamp > license_queue[max_timestamp_index].timestamp) {
                max_timestamp_index = i;
            }
        }

        // Attribuer la licence au client
        printf("Serveur : Attribution de la licence %d au client %d\n", license_queue[max_timestamp_index].license_number, license_queue[max_timestamp_index].client_id);

        // Supprimer la demande de licence de la file d'attente
        for (int i = max_timestamp_index; i < queue_size - 1; ++i) {
            license_queue[i] = license_queue[i + 1];
        }

        queue_size--;
    }
}

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

void check_license_usage() {
    for (int i = 0; i < queue_size; ++i) {
        time_t current_time = time(NULL);
        if (current_time - license_queue[i].timestamp > TIME_LIMIT) {
            // Le temps d'utilisation a dépassé la limite, interroger le client
            printf("Serveur : Interrogation du client %d pour la licence %d\n", license_queue[i].client_id, license_queue[i].license_number);

            // Envoyer une demande au client via le pipe
            write(pipe_fd[1], &license_queue[i], sizeof(LicenseRequest));
        }
    }
}

void license_checker() {
    while (1) {
        sleep(CHECK_INTERVAL);
        P(sem_id);
        check_license_usage();
        V(sem_id);
    }
}

void server() {
    // Création du pipe
    if (pipe(pipe_fd) == -1) {
        perror("Erreur lors de la création du pipe");
        exit(EXIT_FAILURE);
    }

    // Créer le processus de vérification des licences
    if (fork() == 0) {
        close(pipe_fd[1]);  // Fermer l'extrémité d'écriture du pipe dans le processus de vérification
        license_checker();
    }

    close(pipe_fd[0]);  // Fermer l'extrémité de lecture du pipe dans le processus principal

    while (1) {
        // Attente d'un client
        P(sem_id);

        // Vérifier l'utilisation des licences et interroger le client si nécessaire
        check_license_usage();

        if (licenses_available > 0) {
            // Accord de la licence au client
            printf("Serveur : Attribution de la licence %d au client\n", next_license_number);
            licenses_available--;
        } else {
            // Aucune licence disponible, ajouter le client à la file d'attente
            printf("Serveur : Aucune licence disponible, le client est en attente\n");
            enqueue_request(getpid());
            V(sem_id); // Libération pour éviter la famine

            // Attente d'une licence
            P(sem_id);
            dequeue_request();
        }

        // Libération du client
        V(sem_id);
    }
}

int main() {
    // Création des sémaphores
    sem_id = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT);
    semctl(sem_id, 0, SETVAL, 1);

    // Allocation de la file d'attente
    license_queue = (LicenseRequest*)malloc(queue_capacity * sizeof(LicenseRequest));

    // Création du processus serveur
    if (fork() == 0) {
        server();
    }

    // Attente de la fin du processus serveur
    wait(NULL);

    // Fermeture du pipe dans le processus principal
    close(pipe_fd[1]);

    // Suppression des sémaphores
    semctl(sem_id, 0, IPC_RMID);

    // Libération de la file d'attente
    free(license_queue);

    return 0;
}

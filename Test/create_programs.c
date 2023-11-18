#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    int pipe_fd[2];

    // Creazione della pipe
    if (pipe(pipe_fd) == -1) {
        perror("Errore durante la creazione della pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid1, pid2;

    // Creazione del primo programma
    pid1 = fork();

    if (pid1 == 0) {
        // Codice del primo programma
        close(pipe_fd[0]); // Chiudi il lato di lettura della pipe
        dup2(pipe_fd[1], STDOUT_FILENO); // Collega STDOUT alla pipe
        close(pipe_fd[1]); // Chiudi il lato di scrittura della pipe

        execlp("gnome-terminal", "gnome-terminal", "--", "./programma2", NULL);
        perror("Errore durante l'esecuzione di gnome-terminal/programma2");
        exit(EXIT_FAILURE);
    } else if (pid1 < 0) {
        perror("Errore durante la creazione di gnome-terminal/programma2");
        exit(EXIT_FAILURE);
    }



    // Creazione del secondo programma
    pid2 = fork();

    if (pid2 == 0) {
        // Codice del secondo programma
        close(pipe_fd[1]); // Chiudi il lato di scrittura della pipe
        dup2(pipe_fd[0], STDIN_FILENO); // Collega STDIN alla pipe
        close(pipe_fd[0]); // Chiudi il lato di lettura della pipe

        execlp("gnome-terminal", "gnome-terminal", "--", "./programma3", NULL);
        perror("Errore durante l'esecuzione di gnome-terminal/programma3");
        exit(EXIT_FAILURE);
    } else if (pid2 < 0) {
        perror("Errore durante la creazione di gnome-terminal/programma3");
        exit(EXIT_FAILURE);
    }

    // Chiudi entrambi i lati della pipe nel processo principale
    close(pipe_fd[0]);
    close(pipe_fd[1]);

    // Attendi che il secondo programma termini
    waitpid(pid2, NULL, 0);

    return 0;
}

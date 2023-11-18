#include <stdio.h>
#include <string.h>

int main() {
    char input[100];
    while (1)
    {
       // Ricevi l'input dalla pipe dal primo programma
    while (fgets(input, sizeof(input), stdin) != NULL) {
        // Rimuovi il carattere newline dalla stringa letta
        input[strcspn(input, "\n")] = '\0';

        // Stampa l'input ricevuto
        printf("Input ricevuto: %s\n", input);
    }
    }
    

    return 0;
}

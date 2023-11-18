#include <stdio.h>
#include <string.h>

int main() {
    char input[100];
    while (1)
    {
        // Leggi l'input da tastiera
    printf("Inserisci un input: ");
    fgets(input, sizeof(input), stdin);

    // Rimuovi il carattere newline dalla stringa letta
    input[strcspn(input, "\n")] = '\0';

    // Invia l'input attraverso la pipe al secondo programma
    printf("%s", input);
    }
    


    return 0;
}

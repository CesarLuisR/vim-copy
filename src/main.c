#include <stdio.h>

void read_file(char* file_to_open) {
    FILE* file = fopen(file_to_open, "r");

    if (file == NULL) {
        printf("Error opening the file");
        return;
    } else {
        printf("File openend\n");
        fclose(file);
    }
}

int main(int argc, char* argv[]) {
    char* file_to_open;

    if (argv[1]) file_to_open = argv[1];
    else file_to_open = argv[0];

    read_file(file_to_open);

    return 0;
}
#include <stdio.h>
#include <conio.h>
#include <windows.h>
#include <stdlib.h>

typedef struct {
    int width;
    int height;
} windows_size_t;

typedef struct {
    int x;
    int y;
} cursor_pos_t;

windows_size_t get_windows_size() {
    // Obtener el manejador de la consola
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    // Estructura para almacenar la información de la consola
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    
    // Obtener la información de la consola
    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
    
    // Extraer el tamaño de la consola
    int width = consoleInfo.srWindow.Right - consoleInfo.srWindow.Left + 1;
    int height = consoleInfo.srWindow.Bottom - consoleInfo.srWindow.Top + 1;

    windows_size_t w_size = { width, height };

    return w_size;
}

cursor_pos_t arrow_handler(cursor_pos_t c_pos) {
    windows_size_t w_size = get_windows_size();

    int c = _getch();
    switch (c) {
        case 72: // Up
            // If isnt the first line goes up
            if (c_pos.y != 1) c_pos.y = c_pos.y - 1;
            break;
        case 80: // Down
            c_pos.y = c_pos.y + 1;
            break;
        case 75: // Left
            // At beginning of line goes to the previous
            if (c_pos.x == 1 && c_pos.y != 1) {
                c_pos.y = c_pos.y - 1;
                c_pos.x = w_size.width;
                break;
            }

            // If is not begin of line. Goes back.
            if (c_pos.x != 1) c_pos.x = c_pos.x - 1;
            break;
        case 77:
            // If end of line continue in next
            if (c_pos.x == w_size.width) {
                c_pos.y = c_pos.y + 1;
                c_pos.x = 1;
                break;
            }

            c_pos.x = c_pos.x + 1;
            break;
    }

    return c_pos;
}

void edit_mode_keypress_dect(cursor_pos_t c_pos) {
    windows_size_t w_size = get_windows_size();
    int esc_pressed = 0;

    while (1) {
        if (esc_pressed == 1) break;
        int c = _getch();

        switch (c) {
            case 27: // Escape
                esc_pressed = 1;
                break;
            case 224:
                c_pos = arrow_handler(c_pos);
                break;
            case 13: // Enter
                c_pos.y = c_pos.y + 1;
                c_pos.x = 1;
                break;
            case 8: // Backspace
                // If cursor is at the beginning of line. Goes up
                if (c_pos.x == 1 && c_pos.y != 1) {
                    c_pos.y = c_pos.y - 1;
                    c_pos.x = w_size.width;
                    break;
                }

                // If is not begin of line. Goes back.
                if (c_pos.x != 1) c_pos.x = c_pos.x - 1;

                // Print cursor position
                printf("\033[%d;%dH", c_pos.y, c_pos.x);

                // Changes the pos for " "
                printf(" ");
                break;
            default: // Any letter

                // If end of line continue in next
                if (c_pos.x == w_size.width) {
                    c_pos.y = c_pos.y + 1;
                    c_pos.x = 1;

                    printf("\033[%d;%dH", c_pos.y, c_pos.x);

                    // I DONT REMEMBER WHY I ADDED THIS:
                    // printf("%c", c);
                    // c_pos.x = c_pos.x + 1;
                    // break;
                }

                // If writing just after deleting
                if (c_pos.x >= w_size.width) {
                    c_pos.x = c_pos.x - 1;
                    printf("\033[%d;%dH", c_pos.y, c_pos.x);
                }

                printf("%c", c);
                c_pos.x = c_pos.x + 1;

                break;
        }

        printf("\033[%d;%dH", c_pos.y, c_pos.x);
    }
}

typedef struct {
    char** lines;
    long int n_lines;
} document_t;

document_t* create_doc(int n_lines) {
    document_t* doc = malloc(sizeof(document_t));

    doc->n_lines = n_lines;
    doc->lines = malloc(sizeof(char*) * doc->n_lines); 

    for (int i = 0; i < doc->n_lines; i++) 
        doc->lines[i] = malloc(sizeof(char) * 256);
    
    return doc;
}

void doc_addline(document_t* doc, char line[256], int index) {
    if (doc->n_lines < index) {
        doc->n_lines *= 2;
        doc->lines = realloc(doc->lines, doc->n_lines * sizeof(char*));
    }

    strcpy(doc->lines[index], line);
}

void free_doc(document_t* doc) {
    for (int i = 0; i < doc->n_lines; i++) 
        free(doc->lines[i]);
    free(doc->lines);
    free(doc);
}

void read_view_text(document_t* doc) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD coord;
    DWORD charsRead;

    char buffer[256];
    int line_n = 0;

    while (1) {
        coord.X = 0;
        coord.Y = line_n;

        if (ReadConsoleOutputCharacter(hConsole, buffer, sizeof(buffer) - 1, coord, &charsRead)) {
            if (charsRead == 0) break;

            buffer[charsRead] = '\0';
            strcpy(doc->lines[line_n], buffer);
            line_n++;
        } else break;
    }
    doc->n_lines = line_n;
}

int main(void) {
    printf("\033[2J");

    cursor_pos_t init_pos = {1, 1};
    printf("\033[%d;%dH", init_pos.y, init_pos.x);

    edit_mode_keypress_dect(init_pos);

    document_t* doc = create_doc(50);
    // New code
    read_view_text(doc);

    system("cls");
    printf("%s", "The file content: \n\n");
    for (int i = 0; i < doc->n_lines; i++) {
        printf("%s", doc->lines[i]);
    }

    FILE* file = fopen("n.txt", "a");  // Abrir en modo de agregar (append)
    for (int i = 0; i < doc->n_lines; i++) {
        fprintf(file, "%s", doc->lines[i]);  // Agregar cada línea al archivo
    }
    fclose(file);

    free_doc(doc);

    return 0;
}
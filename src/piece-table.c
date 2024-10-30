#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <math.h>
#include <windows.h>

typedef struct {
    char source[9];
    int index;
    int length;
} data_t;

typedef struct piece_t {
    struct piece_t* next;
    struct piece_t* prev;
    data_t* data;
} piece_t;

typedef struct {
    piece_t* head;
    piece_t* tail;
} sequence_t;

typedef struct {
    char* original_buffer;
    char* append_buffer;
    sequence_t* sequence;
} piece_table_t;

typedef struct {
    piece_t* insert_point;
    piece_t* end_point;
    sequence_t* sequence;
} undo_info_t;

typedef struct stack_node_t {
    struct stack_node_t* next;
    undo_info_t* value;
} stack_node_t;

typedef struct {
    stack_node_t* head;
} stack_t;

stack_t* stack_create() {
    stack_t* stack = malloc(sizeof(stack_t));
    stack->head = NULL;
    return stack;
}

void stack_push(stack_t* stack, undo_info_t* info) {
    stack_node_t* node = malloc(sizeof(stack_node_t));
    node->next = NULL;
    node->value = info; 

    if (stack->head == NULL) {
        stack->head = node;
    } else {
        node->next = stack->head;
        stack->head = node;
    }
}

void stack_pop(stack_t* stack) {
    if (stack->head == NULL) return;

    if (stack->head->next == NULL) {
        stack->head = NULL;
    } else {
        stack_node_t* current = stack->head;
        stack->head = stack->head->next;
        free(current->value);
        free(current);
    }
}

void stack_dump(stack_t* stack, piece_table_t* pt) {
    if (stack->head == NULL) {
        printf("Stack vacia\n");
        return;
    }

    piece_t* current = stack->head->value->sequence->head;

    const char* buffer = (strcmp(current->data->source, "original") == 0)
        ? pt->original_buffer
        : pt->append_buffer;
        
    for (int i = 0; i < current->data->length; i++) 
        putchar(buffer[current->data->index + i]);

    printf("STACK DUMP: ");
    printf("\n");
};

piece_t* create_piece(const char source[9], int index, int length) {
    piece_t* piece = malloc(sizeof(piece_t));
    piece->data = malloc(sizeof(data_t));

    strcpy(piece->data->source, source);
    piece->data->index = index;
    piece->data->length = length;

    piece->next = NULL;
    piece->prev = NULL;

    return piece;
}

undo_info_t* create_info(piece_table_t* pt, sequence_t* pieces) {
    sequence_t* seq_range = malloc(sizeof(sequence_t));
    seq_range->tail = NULL;

    piece_t* insert_point = NULL; 
    piece_t* end_point = NULL;

    // With all of this modifications i dont know if this still works fine
    if (pieces->head) {
        insert_point = pieces->head->prev ? pieces->head->prev : pt->sequence->head;
    } else {
        insert_point = pt->sequence->head;
    }

    // does not even magaro understand this, but it works, so... 
    if (pieces->tail) {
        end_point = pieces->tail->next;
    } else {
        if (pieces->head) {
            end_point = pieces->head->next ? pieces->head->next : pt->sequence->tail;
        } else {
            end_point = pt->sequence->tail;
        }
    }

    undo_info_t* info = malloc(sizeof(undo_info_t));

    piece_t* piece = pieces->head;
    while (piece != NULL) {
        // i don't remember why i did this code, couldnt i just use the pieces sequence???
        piece_t* new_piece = create_piece(piece->data->source, piece->data->index, piece->data->length);

        if (seq_range->tail == NULL) {
            seq_range->head = new_piece;
            seq_range->tail = new_piece;
            new_piece->prev = NULL;
        } else {
            new_piece->prev = seq_range->tail;
            seq_range->tail = new_piece;
            new_piece->prev->next = new_piece;
        }

        info->insert_point = insert_point;
        info->end_point = end_point;
        info->sequence = seq_range;

        piece = piece->next;
    }

    return info;
}

piece_table_t* pt_create(const char* file_text) {
    piece_table_t* pt = malloc(sizeof(piece_table_t));

    pt->original_buffer = strdup(file_text);
    pt->append_buffer = NULL;
    pt->sequence = malloc(sizeof(sequence_t));

    piece_t* initial_piece = create_piece("original", 0, strlen(file_text) + 1);
    pt->sequence->head = pt->sequence->tail = initial_piece;

    return pt;
};

void display_text(piece_table_t* pt) {
    piece_t* current = pt->sequence->head;

    printf("\n");
    while (current != NULL) {
        const char* buffer = (strcmp(current->data->source, "original") == 0)
            ? pt->original_buffer
            : pt->append_buffer;
        
        printf("PIECE: [");
        for (int i = 0; i < current->data->length; i++) 
            putchar(buffer[current->data->index + i]);
        printf("] -> length: %d\n", current->data->length);

        current = current->next;
    }

    putchar('\n');
}

void d_piece(piece_t* piece, piece_table_t* pt) {
    const char* buffer = (strcmp(piece->data->source, "original") == 0)
        ? pt->original_buffer
        : pt->append_buffer;
        
    printf("ONLY PIECE: [");
    for (int i = 0; i < piece->data->length; i++) 
        putchar(buffer[piece->data->index + i]);
    printf("] -> length: %d\n", piece->data->length);
}

void insert_text(piece_table_t* pt, char* new_text, int position, stack_t* stack) {
    int new_text_len = strlen(new_text);

    if (pt->append_buffer == NULL) {
        pt->append_buffer = malloc(new_text_len + 1);
        strcpy(pt->append_buffer, "");
    } else {
        pt->append_buffer = realloc(pt->append_buffer, strlen(pt->append_buffer) + new_text_len + 1);
    }

    strcat(pt->append_buffer, new_text);

    piece_t* current = pt->sequence->head;
    int cur_pos = 0;

    while (current != NULL) {
        if (cur_pos + current->data->length > position) {
            // creating and adding undo info
            sequence_t* undo_seq = malloc(sizeof(sequence_t));
            undo_seq->head = current;
            undo_seq->tail = NULL;

            undo_info_t* info = create_info(pt, undo_seq);
            stack_push(stack, info);

            // insert login ->
            int split_text = position - cur_pos;

            piece_t* after_piece = NULL;
            if (current->data->length == split_text) {
                after_piece = current->next;
            } else {
                after_piece = create_piece(current->data->source, current->data->index + split_text, current->data->length - split_text);
                after_piece->next = current->next;
            }

            // Cutting the first half 
            current->data->length = split_text;
 
            if (current->next != NULL) {
                current->next->prev = after_piece;
            }

            piece_t* new_piece = create_piece("append", strlen(pt->append_buffer) - new_text_len, new_text_len);

            // if piece is empty (current piece was split in the index 0)
            if (current->data->length == 0) {
                if (current->prev) {
                    new_piece->prev = current->prev;
                    current->prev->next = new_piece;
                } 

                if (current == pt->sequence->head)
                    pt->sequence->head = new_piece;
                
                new_piece->next = after_piece;
                after_piece->prev = new_piece;
                if (current->next == pt->sequence->tail) 
                    pt->sequence->tail = after_piece;

                return;
            }

            current->next = new_piece;
            new_piece->prev = current;
            new_piece->next = after_piece;
            after_piece->prev = new_piece;

            if (current->next == pt->sequence->tail) {
                pt->sequence->tail = after_piece;
            }

            return;
        }

        cur_pos += current->data->length;
        current = current->next;
    }
}

void delete_text(piece_table_t* pt, int index, int length, stack_t* stack) {
    piece_t* current = pt->sequence->head;
    int cur_pos = 0;

    piece_t *first_piece = NULL, *last_piece = NULL;

    // init the undo_seq that is the sequence of pieces that will go to the undo stack 
    sequence_t* undo_seq = malloc(sizeof(sequence_t));
    undo_seq->tail = NULL;
    undo_seq->head = NULL;

    while (current != NULL) {
        int piece_start = cur_pos;
        int piece_end = 0;
        if (current->data->length != 0) {
            piece_end = cur_pos + current->data->length - 1;
        } else {
            piece_end = cur_pos;
        }

        // When deleting is in the same piece
        if (index >= piece_start && index + length <= piece_end) {
            piece_t* first_span = create_piece(current->data->source, current->data->index, index - piece_start);
            piece_t* last_span = create_piece(current->data->source, current->data->index + (index + length - piece_start), piece_end - (index + length));

            first_span->next = last_span;
            last_span->prev = first_span;

            if (current->prev) {
                current->prev->next = first_span;
                first_span->prev = current->prev;
            } else {
                pt->sequence->head = first_span;
                first_span->prev = NULL;
            }

            if (current->next) {
                current->next->prev = last_span;
                last_span->next = current->next;
            } else {
                pt->sequence->tail = last_span;
                last_span->next = NULL;
            }

            // creating and adding the undo info to the stack
            piece_t* piece = create_piece(current->data->source, current->data->index, current->data->length);
            undo_seq->head = piece;
            undo_seq->tail = piece;
            piece->next = current->next;
            piece->prev = current->prev;

            undo_info_t* info = create_info(pt, undo_seq);
            stack_push(stack, info);

            free(current->data);
            free(current);
            return;
        }

        // When deleting is on various pieces

        // Beginning of the deletion piece

        if (index >= piece_start && index <= piece_end) {
            first_piece = create_piece(current->data->source, current->data->index, index - piece_start);

            if (current->prev) {
                current->prev->next = first_piece;
                first_piece->prev = current->prev;
            } else {
                pt->sequence->head = first_piece;
                first_piece->prev = NULL;
            }

            first_piece->next = current;
            current->prev = first_piece;
        }

        // End of the deletion piece
        if (index + length >= piece_start && index + length <= piece_end) {
            last_piece = create_piece(current->data->source, current->data->index + (index + length - piece_start), piece_end - (index + length) + 1);

            if (current->next) {
                current->next->prev = last_piece;
                last_piece->next = current->next;
            } else {
                pt->sequence->tail = last_piece;
                last_piece->next = NULL;  
            }

            last_piece->prev = current;
            current->next = last_piece;
        }

        // Pieces bewteen. For delete
        if (index + length > piece_start && index < piece_end) {
            piece_t* to_delete = current;

            // creating undo info to the stack
            piece_t* piece = create_piece(current->data->source, current->data->index, current->data->length);

            if (undo_seq->tail == NULL) {
                undo_seq->tail = piece;
                undo_seq->head = piece;
                piece->next = NULL;  
                piece->prev = NULL;
            } else {
                piece->prev = undo_seq->tail;  
                undo_seq->tail->next = piece;  
                undo_seq->tail = piece;
                piece->next = current->next;
            }

            if (pt->sequence->tail == undo_seq->tail->prev) {
                pt->sequence->tail = undo_seq->tail;
            }

            free(to_delete->data);
            free(to_delete);
            to_delete = NULL;
        }

        cur_pos += current->data->length;
        current = current->next;
    }

    // adding undo info to the stack
    undo_info_t* info = create_info(pt, undo_seq);
    stack_push(stack, info);

    if (first_piece && last_piece) {
        first_piece->next = last_piece;
        last_piece->prev = first_piece;
    }
}

void replace_text(piece_table_t* pt, int index, const char* new_text, stack_t* stack) {
    delete_text(pt, index, strlen(new_text), stack);
    insert_text(pt, new_text, index, stack);
}

// Undo/redo
void undo(piece_table_t* pt, stack_t* stack) {
    if (stack->head == NULL) return;

    piece_t* current = pt->sequence->head;
    piece_t* stack_head = stack->head->value->sequence->head;
    piece_t* insert_point = stack->head->value->insert_point;
    piece_t* end_point = stack->head->value->end_point;

    while (current != NULL) {
        // if there is only one piece in the sequence
        if (insert_point == end_point) {
            pt->sequence->head = stack->head->value->sequence->head;
            pt->sequence->tail = stack->head->value->sequence->head;
            pt->sequence->tail->next = NULL;
            stack_pop(stack);
            return;
        }

        // else
        if (current == insert_point) {
            current->next = stack->head->value->sequence->head;

            if (current->next->next == pt->sequence->tail)
                pt->sequence->tail = stack->head->value->sequence->head;
            
            stack_pop(stack);
            return;
        }

        current = current->next;
    }
}

typedef struct {
    void** items;   
    int capacity;
    int length;
    char type;
} Vector;

Vector* create_array(int capacity, char type) {
    Vector* vec = malloc(sizeof(Vector));
    vec->capacity = capacity;
    vec->length = 0;
    vec->type = type;

    if (type == 's') {
        vec->items = malloc(1000 * sizeof(char));
        strcpy((char*)vec->items, "");
    } else {
        vec->items = malloc(capacity * sizeof(void*));
    }

    return vec;
}

void array_push(Vector* vec, void* value) {
    if (vec->type == 's') {
        int value_length = strlen(value);
    
        while (vec->length + value_length + 1 > vec->capacity) {
            vec->capacity *= 2;
            vec->items = realloc(vec->items, vec->capacity * sizeof(char));
            if (vec->items == NULL) {
                printf("Error reallocating memory\n");
                return; 
            }
        }

        strcat((char*)vec->items, value);
        vec->length += value_length;
    } else {
        if (vec->length >= vec->capacity) {
            vec->capacity *= 2;
            vec->items = realloc(vec->items, vec->capacity * sizeof(void*));
            if (vec->items == NULL) {
                printf("Error reallocating memory\n");
                return;
            }
        }

        vec->items[vec->length] = value;
        vec->length++;
    }
}

void array_remove(Vector* vec, int index) {
    if (index < 0 || index > vec->length) return;
    
    for (int i = index; i < vec->length - 1; i++)
        ((char*)vec->items)[i] = ((char*)vec->items)[i+1];

    vec->length--;
}

void array_insert(Vector* vec, int index, int value) {
    if (index < 0 || index > vec->length) return;

    for (int i = vec->length; i > index; i--) 
        ((char*)vec->items)[i] = ((char*)vec->items)[i - 1];
    
    vec->length++;

    ((char*)vec->items)[index] = value;
}

void printList(Vector* vec) {
    for (int i = 0; i < vec->length; i++) 
        printf("%d, ", ((char*)vec->items)[i]);

    printf("\nVector capacity => %d, vector length => %d", vec->capacity, vec->length);
}

FILE* read_file(char* file_to_open) {
    FILE* file = fopen(file_to_open, "r");

    if (file == NULL) {
        printf("Error opening the file");
        return NULL;
    } else {
        printf("File openend\n");
    }

    return file;
}

typedef struct {
    int width;
    int height;
} windows_size_t;

typedef struct {
    int x;
    int y;
} cursor_pos_t;

windows_size_t get_windows_size() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    
    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
    
    int width = consoleInfo.srWindow.Right - consoleInfo.srWindow.Left + 1;
    int height = consoleInfo.srWindow.Bottom - consoleInfo.srWindow.Top + 1;

    windows_size_t w_size = { width, height };

    return w_size;
}

void gotoxy(int x, int y) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD coord = {x, y};
    SetConsoleCursorPosition(hConsole, coord);
}

void draw_screen(windows_size_t w_size, piece_table_t* pt, int current_pos, Vector* row_info) {
    printf("\033[H");
    printf("\033[2J");

    piece_t* current = pt->sequence->head;

    char** view_text = (char**)malloc(sizeof(char*) * 1000);  
    int view_text_size = 0;
    int line_n = 1;

    row_info->items = realloc(NULL, row_info->capacity * sizeof(void*));
    row_info->capacity = 1000;
    row_info->length = 0;
    char pt_line[256] = "";

    while (current) {
        const char* buffer = (strcmp(current->data->source, "original") == 0)
            ? pt->original_buffer
            : pt->append_buffer;

        for (int i = 0; i < current->data->length; i++) {
            char c = buffer[current->data->index + i];

            if (c == '\n') {
                char full_line[300];
                sprintf(full_line, "%4d   %s", line_n, pt_line);

                int full_line_len = strlen(pt_line);
                array_push(row_info, (int*)full_line_len);

                char* line_copy = malloc(strlen(full_line) + 1);
                if (line_copy) {
                    strcpy(line_copy, full_line);
                    view_text[view_text_size++] = line_copy; 
                }

                pt_line[0] = '\0';  
                line_n++;
            } else {
                int len = strlen(pt_line);
                pt_line[len] = c;
                pt_line[len + 1] = '\0';  
            }
        }

        current = current->next;
    }

    if (strlen(pt_line) > 0) {
        char full_line[300];
        sprintf(full_line, "%4d   %s", line_n, pt_line);

        char* line_copy = malloc(strlen(full_line) + 1);
        if (line_copy) {
            strcpy(line_copy, full_line);
            view_text[view_text_size++] = line_copy;  
        }

        int full_line_len = strlen(pt_line);
        array_push(row_info, (int*)full_line_len);
    }

    // aqui va el nuevo codigo
    for (int i = current_pos; i < w_size.height + current_pos - 1; i++) {
        printf("%s\n", view_text[i]);
    }

    for (int i = 0; i < view_text_size; i++) {
        free(view_text[i]);
    }

    free(view_text);
}

cursor_pos_t arrow_handler(cursor_pos_t c_pos, int* text_line, piece_table_t* pt, Vector* row_info) {
    windows_size_t w_size = get_windows_size();
    const int init_x = 8;

    int c = _getch();
    switch (c) {
        case 72: {
            // Up
            // If isnt the first line goes up
            if (c_pos.y != 1) {
                if (c_pos.y != 5 || *text_line == 5) c_pos.y = c_pos.y - 1;
                else draw_screen(w_size, pt, *text_line - 6, row_info);

                if (*text_line - 1 > 0) *text_line = *text_line - 1;

                void* new_pos = row_info->items[*text_line - 1];
                if (c_pos.x - init_x > row_info->items[*text_line - 1]) {
                    c_pos.x = (int)new_pos;
                    c_pos.x += init_x;
                }
            }
            break;
        }
        case 80: // Down
            if (c_pos.y != w_size.height - 5) c_pos.y = c_pos.y + 1;
            else draw_screen(w_size, pt, *text_line - (w_size.height - 6), row_info);

            *text_line = *text_line + 1;

            void* new_pos = row_info->items[*text_line - 1];
            if (c_pos.x - init_x > row_info->items[*text_line - 1]) {
                c_pos.x = (int)new_pos;
                c_pos.x += init_x;
            }

            break;
        case 75: // Left
            // At beginning of line goes to the previous
            if (c_pos.x == init_x && c_pos.y != 1) {
                break;
            }

            // If is not begin of line. Goes back.
            if (c_pos.x != init_x) c_pos.x = c_pos.x - 1;
            break;
        case 77:
            // If end of line continue in next
            if (c_pos.x - init_x == row_info->items[*text_line - 1]) {
                break;
            }

            c_pos.x = c_pos.x + 1;
            break;
    }

    return c_pos;
}

void edit_mode_keypress_dect(piece_table_t* pt, Vector* row_info, stack_t* stack) {
    cursor_pos_t c_pos = {8, 1};
    int text_line_np = 1;
    int *text_line = &text_line_np;
    int esc_pressed = 0;
    const int init_x = 8;

    while (1) {
        windows_size_t w_size = get_windows_size();
        if (esc_pressed == 1) break;
        int c = _getch();
        int pt_cp = 0;

        for (int i = 0; i < *text_line - 1; i++) {
            if (*text_line > 0) pt_cp += 1;

            pt_cp += (int)row_info->items[i]; 
        }
        pt_cp += c_pos.x - init_x;


        switch (c) {
            case 27: // Escape
                esc_pressed = 1;
                break;
            case 224:
                c_pos = arrow_handler(c_pos, text_line, pt, row_info);
                break;
            case 13: // Enter
                insert_text(pt, "\n\0", pt_cp, stack);
                draw_screen(w_size, pt, *text_line - c_pos.y, row_info);
                //c_pos.y = c_pos.y + 1;
                //c_pos.x = init_x;
                break;
            case 8: // Backspace
                if (pt_cp - 1 >= 0 && c_pos.x - 1 >= init_x) {
                    delete_text(pt, pt_cp - 1, 1, stack);
                    draw_screen(w_size, pt, *text_line - c_pos.y, row_info);
                    c_pos.x = c_pos.x - 1;
                }

                // If is not begin of line. Goes back.
                // a lot of bugs
                if (c_pos.x == init_x && c_pos.y != 1) {
                    c_pos.y = c_pos.y - 1;
                    *text_line = *text_line - 1;
                    c_pos.x = (int)row_info->items[*text_line - 1];
                    c_pos.x += init_x;
                    break;
                }

                break;
            case 26:
                undo(pt, stack);
                draw_screen(w_size, pt, *text_line - c_pos.y, row_info);

                break;
            default: // Any letter

                // If end of line continue in next
                if (c_pos.x == w_size.width) {
                    c_pos.y = c_pos.y + 1;
                    c_pos.x = init_x;

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

        //printf("\033[2J");
        //display_text(pt);
                    //printf("X: %d, RC: %d", c_pos.x, pt_cp);
        printf("\033[%d;%dH", c_pos.y, c_pos.x);
        // printf("LA LINEA: %d", *text_line);
    }
}

int main(int argc, char* argv[]) {
    char* file_to_open = NULL;

    if (argc > 1) {
        file_to_open = argv[1];
    } else {
        printf("You must spicify a file to open.\n");
        return 1;
    }

    FILE* file = read_file(file_to_open);
    if (file == NULL) return 1;

    Vector* original_buffer = create_array(1000, 's');
    char file_line[256] = ""; 
    while(fgets(file_line, sizeof(file_line), file)) {
        array_push(original_buffer, file_line);
    }
    fclose(file);

    piece_table_t* pt = pt_create((char*)original_buffer->items);
    stack_t* stack = stack_create();

    Vector* row_info = create_array(1000, 'n');
    windows_size_t w_size = get_windows_size();
    draw_screen(w_size, pt, 0, row_info);
    printf("\033[%d;%dH", 0, 8);
    edit_mode_keypress_dect(pt, row_info, stack);

    //delete_text(pt, 11, 1, stack);
    //delete_text(pt, 10, 1, stack);
    //delete_text(pt, 20, 1, stack);
    //display_text(pt);
    return 0;
}
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct stack_node_t {
    struct stack_node_t* next;
    const char* value;
} stack_node_t;

typedef struct {
    stack_node_t* head;
} stack_t;

stack_t* stack_create() {
    stack_t* stack = malloc(sizeof(stack_t));
    stack->head = NULL;
    return stack;
}

void stack_push(stack_t* stack, const char* value) {
    stack_node_t* node = malloc(sizeof(stack_node_t));
    node->next = NULL;
    node->value = strdup(value);

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

void stack_dump(stack_t* stack) {
    if (stack->head != NULL) {
        printf("El valor es: %s\n", stack->head->value);
    } else {
        printf("Stack vacia\n");
    }
};

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
    piece_t** entries;
} sequence_t;

typedef struct {
    char* original_buffer;
    char* append_buffer;
    sequence_t* sequence;
} piece_table_t;

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

piece_table_t* pt_create(const char* file_text) {
    piece_table_t* pt = malloc(sizeof(piece_table_t));

    pt->original_buffer = strdup(file_text);
    pt->append_buffer = NULL;
    pt->sequence = malloc(sizeof(sequence_t));
    pt->sequence->entries = NULL;

    piece_t* initial_piece = create_piece("original", 0, strlen(file_text));
    pt->sequence->head = pt->sequence->tail = initial_piece;

    return pt;
};

void display_text(piece_table_t* pt) {
    piece_t* current = pt->sequence->head;
    printf("APPEND: %s\n\n", pt->append_buffer);

    while (current != NULL) {
        const char* buffer = (strcmp(current->data->source, "original") == 0)
            ? pt->original_buffer
            : pt->append_buffer;
        
        for (int i = 0; i < current->data->length; i++) 
            putchar(buffer[current->data->index + i]);

        current = current->next;
    }

    putchar('\n');
}

void insert_text(piece_table_t* pt, const char* new_text, int position) {
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
        if (cur_pos + current->data->length >= position) {
            int split_text = position - cur_pos;

            piece_t* after_piece = create_piece(current->data->source, current->data->index + split_text, current->data->length - split_text);

            // Cutting the first half 
            current->data->length = split_text;

            after_piece->next = current->next;
            if (current->next != NULL) {
                current->next->prev = after_piece;
            }

            piece_t* new_piece = create_piece("append", strlen(pt->append_buffer) - new_text_len, new_text_len);

            if (current->data->length == 0) {
                if (current->prev)
                    current->prev->next = new_piece;

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

void delete_text(piece_table_t* pt, int index, int length) {
    piece_t* current = pt->sequence->head;
    int cur_pos = 0;
    piece_t *first_piece = NULL, *last_piece = NULL;

    while (current != NULL) {
        int piece_start = cur_pos;
        int piece_end = cur_pos + current->data->length;

        if (index >= piece_start && index + length <= piece_end) {
            piece_t* first_span = create_piece(current->data->source, current->data->index, index - piece_start);
            piece_t* last_span = create_piece(current->data->source, current->data->index + (index + length - piece_start), piece_end - (index + length));

            first_span->next = last_span;
            last_span->prev = first_span;

            if (current->prev != NULL) current->prev->next = first_span;
            else pt->sequence->head = first_span;
            
            if (current->next != NULL) {
                last_span->next = current->next;
                current->next->prev = last_span;
            } else pt->sequence->tail = last_span;

            free(current->data);
            free(current);
            return;
        }

        if (index >= piece_start && index < piece_end) {
            first_piece = create_piece(current->data->source, current->data->index, index - piece_start);

            if (current->prev) {
                current->prev->next = first_piece;
                first_piece->prev = current->prev;
            } else pt->sequence->head = first_piece;

            first_piece->next = NULL;
        }

        if (index + length >= piece_start && index + length <= piece_end) {
            last_piece = create_piece(current->data->source, current->data->index + (index + length - piece_start), piece_end - (index + length));

            if (current->next) {
                current->next->prev = last_piece;
                last_piece->next = current->next;
            } else pt->sequence->tail = last_piece;

            last_piece->prev = NULL;
        }

        if (index + length > piece_start && index < piece_end) {
            piece_t* to_delete = current;

            free(to_delete->data);
            free(to_delete);
        } 

        cur_pos += current->data->length;
        current = current->next;
    }

    if (first_piece && last_piece) {
        first_piece->next = last_piece;
        last_piece->prev = first_piece;
    }
}

void replace_text(piece_table_t* pt, int index, const char* new_text) {
    delete_text(pt, index, strlen(new_text));
    insert_text(pt, new_text, index);
}

// Undo/redo

int main(void) {
    const char* original = "Hola a todos";
    piece_table_t* pt = pt_create(original);
    stack_t* stack = stack_create();
    // stack_push(stack, "Hola");
    // stack_push(stack, "Despues de hola");
    // stack_push(stack, "Ultimo valor");
    // stack_pop(stack);
    // stack_pop(stack);
    // stack_pop(stack);
    // stack_dump(stack);

    // insert_text(pt, " Joel y a", 6);
    // insert_text(pt, "Al inicio: ", 0);

    // display_text(pt);

    // replace_text(pt, 2, "TODO ESTO VA PARA xdxdxdxdxdODOSaa");
    // replace_text(pt, 0, "CO");

    // display_text(pt);

    return 0;
}
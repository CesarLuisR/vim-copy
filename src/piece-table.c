#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

    piece_t* insert_point = pieces->head->prev ? pieces->head->prev : pt->sequence->head;
    piece_t* end_point = NULL;

    if (pieces->tail) {
        end_point = pieces->tail->next;
    } else {
        end_point = pieces->head->next ? pieces->head->next : pt->sequence->tail;
    }

    undo_info_t* info = malloc(sizeof(undo_info_t));

    piece_t* piece = pieces->head;
    while (piece != NULL) {
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

    while (current != NULL) {
        printf("PIECE: (", current->data->index);
        const char* buffer = (strcmp(current->data->source, "original") == 0)
            ? pt->original_buffer
            : pt->append_buffer;
        
        for (int i = 0; i < current->data->length; i++) 
            putchar(buffer[current->data->index + i]);
        printf("]\n");

        current = current->next;
    }

    putchar('\n');
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
        // here we did a little change with >=
        if (cur_pos + current->data->length > position) {
            sequence_t* undo_seq = malloc(sizeof(sequence_t));
            undo_seq->head = current;
            undo_seq->tail = NULL;
            undo_info_t* info = create_info(pt, undo_seq);
            stack_push(stack, info);

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

    sequence_t* undo_seq = malloc(sizeof(sequence_t));
    undo_seq->tail = NULL;
    undo_seq->head = NULL;

    while (current != NULL) {
        int piece_start = cur_pos;
        int piece_end = cur_pos + current->data->length - 1;

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
                pt->sequence->head->next = first_piece;
            }

            first_piece->next = NULL;
        }

        // End of the deletion piece
        if (index + length >= piece_start && index + length <= piece_end) {
            last_piece = create_piece(current->data->source, current->data->index + (index + length - piece_start), piece_end - (index + length));

            if (current->next) {
                current->next->prev = last_piece;
                last_piece->next = current->next;
            } else pt->sequence->tail = last_piece;

            last_piece->prev = NULL;
        }

        // Pieces bewteen. For delete
        if (index + length > piece_start && index < piece_end) {
            piece_t* to_delete = current;
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
        if (insert_point == end_point) {
            pt->sequence->head = stack->head->value->sequence->head;
            pt->sequence->tail = stack->head->value->sequence->head;
            pt->sequence->tail->next = NULL;
            stack_pop(stack);
            return;
        }

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

int main(void) {
    const char* original = "Hola a todos";
    piece_table_t* pt = pt_create(original);
    stack_t* stack = stack_create();

    insert_text(pt, "Esto deberia ir antes de todo, ", 0, stack);
    insert_text(pt, ", y esto deberia ir al final", 43, stack);
    insert_text(pt, " y esto todavia mas al final xd", 71, stack);
    delete_text(pt, 71, 1, stack);

    undo(pt, stack);
    undo(pt, stack);
    undo(pt, stack);
    undo(pt, stack);

    display_text(pt);
    return 0;
}
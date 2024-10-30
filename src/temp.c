#include <stdlib.h>
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
    piece_t** entries;
} sequence_t;

typedef struct {
    char* original_buffer;
    char* append_buffer;
    sequence_t* sequence;
} piece_table_t;

piece_t* create_piece(const char* source, int index, int length) {
    piece_t* piece = malloc(sizeof(piece_t));
    piece->data = malloc(sizeof(data_t));

    strcpy(piece->data->source, source);
    piece->data->index = index;
    piece->data->length = length;

    piece->next = NULL;
    piece->prev = NULL;

    return piece;
}

void insert_text(piece_table_t* pt, const char* new_text, int position) {
    int new_text_len = strlen(new_text);

    if (pt->append_buffer == NULL) {
        pt->append_buffer = strdup(new_text);
    } else {
        pt->append_buffer = realloc(pt->append_buffer, sizeof(pt->append_buffer) + new_text_len);
    }

    strcat(pt->append_buffer, new_text);

    piece_t* current = pt->sequence->head;
    int cur_pos = 0;

    while (current != NULL) {
        if (cur_pos + current->data->length > position) {
            int split_text = position - current->data->length;

            piece_t* after_piece = create_piece(current->data->source, current->data->index + split_text, current->data->length - split_text);

            // Cutting the first half 
            current->data->length = split_text;

            after_piece->next = current->next;
            if (current->next != NULL) {
                current->next->prev = after_piece;
            }

            piece_t* new_piece = create_piece("append", strlen(pt->append_buffer) - new_text_len, new_text_len);

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

void insert_text(piece_table_t* pt, const char* new_text, int position) {
    int new_text_len = strlen(new_text);

    // Allocate or reallocate the append buffer if needed
    if (pt->append_buffer == NULL) {
        pt->append_buffer = malloc(new_text_len);
    } else {
        pt->append_buffer = realloc(pt->append_buffer, strlen(pt->append_buffer) + new_text_len);
    }

    // Copy the new text into the append buffer
    strcat(pt->append_buffer, new_text);

    // Now, find the position in the piece list and split the piece
    piece_t* current = pt->sequence->head;
    int cur_pos = 0;

    while (current != NULL) {
        if (cur_pos + current->data->length > position) {
            int split_index = position - cur_pos;

            // Create a new piece for the text after the insertion point
            piece_t* after_piece = create_piece(current->data->source, current->data->index + split_index, current->data->length - split_index);

            // Adjust the current piece's length to end at the insertion point
            current->data->length = split_index;

            // Link the after piece
            after_piece->next = current->next;
            if (current->next != NULL) {
                current->next->prev = after_piece;
            }
            current->next = after_piece;
            after_piece->prev = current;

            // Create a new piece for the inserted text
            piece_t* new_piece = create_piece("append", strlen(pt->append_buffer) - new_text_len, new_text_len);

            // Insert the new piece between current and after_piece
            current->next = new_piece;
            new_piece->prev = current;
            new_piece->next = after_piece;
            after_piece->prev = new_piece;

            // If the split was at the end, adjust the tail
            if (current == pt->sequence->tail) {
                pt->sequence->tail = after_piece;
            }

            return;
        }

        cur_pos += current->data->length;
        current = current->next;
    }
}

void remove_text(piece_table_t* pt, int index, int length) {
    piece_t* current = pt->sequence->head;

    int cur_pos = 0;
    piece_t *first_piece = NULL, *last_piece = NULL;

    while (current != NULL) {
        // find the first node and split it
        if (cur_pos + current->data->length > index && first_piece == NULL) {
            int split_text = index - cur_pos;

            first_piece = create_piece(current->data->source, current->data->index, split_text);
            first_piece->next = NULL;
            first_piece->prev = current->prev;

            if (current == pt->sequence->head) {
                pt->sequence->head = first_piece;
            } else if (current->prev != NULL) 
                current->prev->next = first_piece;
        }

        // find the last node and split it
        if (cur_pos + current->data->length > index + length) {
            int split_text = (index + length) - cur_pos;

            last_piece = create_piece(current->data->source, current->data->index + split_text, current->data->length - split_text);

            last_piece->next = current->next;
            last_piece->prev = first_piece;

            if (current->next != NULL) 
                current->next->prev = last_piece;

            if (pt->sequence->tail == current->next)
                pt->sequence->tail = last_piece;

            first_piece->next = last_piece;
        }

        cur_pos += current->data->length;
        current = current->next;
    }
}

    if (piece->prev) {
        seq_range->head = piece->prev;
        seq_range->head->next = new_piece;
    } else {
        seq_range->head = new_piece;
        seq_range->head->next = new_piece->next;
    }

    if (piece->next) {
        new_piece->next = piece->next;
        seq_range->tail = piece->next;
    } else {
        new_piece->next = NULL;
        seq_range->tail = new_piece;
    }
    
    if (seq_range->tail) {
        seq_range->tail->prev = new_piece;
        seq_range->tail->next = NULL;
    }


        if (index >= piece_start && index <= piece_end) {
            first_piece = create_piece(current->data->source, current->data->index, index - piece_start);
                        //d_piece(current->next, pt);

            if (current->prev) {
                current->prev->next = first_piece;
                first_piece->prev = current->prev;
            } else {
                pt->sequence->head->next = first_piece;
            }

            first_piece->next = NULL;
        }
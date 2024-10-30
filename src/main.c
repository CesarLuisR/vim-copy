#include <stdio.h>
#include <stdlib.h>

typedef struct node_t {
    struct node_t* next;
    void* value;
} node_t;

typedef struct {
    node_t* head;
} stack_t;

typedef struct {
    char* name;
    int age;
} data_t;

stack_t* create_stack() {
    stack_t* stack = malloc(sizeof(stack_t));
    stack->head = NULL;
    return stack;
}

void stack_push(stack_t* stack, void* value) {
    if (stack->head) {
        node_t* new_node = malloc(sizeof(node_t));
        stack->head->next = NULL;
        new_node->value = value;
        new_node->next = stack->head;
        stack->head = new_node;
    } else {
        stack->head = malloc(sizeof(node_t));
        stack->head->value = value;
        stack->head->next = NULL;
    }
}

int main(void) {
    stack_t* stack = create_stack();
    int n = 10;
    stack_push(stack, &n);

    void* cur = stack->head->value;
    data_t data;
    data.age = 10;
    data.name = "Cesar";
    printf("%s", data.name);
    return 0;
}
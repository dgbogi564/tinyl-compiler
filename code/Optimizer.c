#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "InstrUtils.h"
#include "Utils.h"

typedef struct identifier {
    int v;                     /* Identifier value */
    struct identifier *next;   /* Next id */
} identifier_t;

typedef struct identifier_list {
    int size;                  /* Size */
    struct identifier *head;   /* Beginning of id list */
    struct identifier *tail;   /* End of id list */
} identifierList_t;

identifier_t * id_create(int v) {
    identifier_t *id = malloc(sizeof(identifier_t));
    id->v = v;
    id->next = NULL;
   return id;
}

identifierList_t * idl_create() {
    identifierList_t *id_list = malloc(sizeof(identifierList_t));
    id_list->size = 0;
    id_list->head = NULL;
    id_list->tail = NULL;
    return id_list;
}

void id_add(identifierList_t *id_list, int v) {
    identifier_t *id = id_create(v), *temp = id_list->head;
    /* check for dupes */
    while(temp != NULL) {
        if(temp->v == v) {
            return;
        }
        temp = temp->next;
    }
    /* insert into id_list */
    if(!id_list->size) {
        id_list->head = id;
        id_list->tail = id;
    } else {
        id->next = id_list->head;
        id_list->head = id;
    }
    id_list->size++;
}

void id_remove(identifierList_t *id_list, int v) {
    identifier_t *id, *prev;
    if(!(id = id_list->head)) {
        return;
    }
    if(id->v == v) {
        id_list->head = id->next;
        free(id);
        id_list->size--;
        return;
    }
    prev = id;
    id = id->next;
    while(id != NULL) {
        if(id->v == v) {
            prev->next = id->next;
            free(id);
            id_list->size--;
            return;
        }
        prev = id;
        id = id->next;
    }
}

void idl_destroy(identifierList_t *id_list) {
    identifier_t *id = id_list->head;
    while(id != NULL) {
        identifier_t *temp = id->next;
        free(id);
        id = temp;
    }
    id_list->head = NULL;
    id_list->tail = NULL;
}

int is_critical(identifierList_t *id_list, Instruction *instr) {
    identifier_t *id = id_list->head;
    int loads_to = instr->field1;
    while(id != NULL) {
        if(id->v == loads_to) {
            return 1;
        }
        id = id->next;
    }
    return 0;
}

int main()
{
	Instruction *head;

	head = ReadInstructionList(stdin);
	if (!head) {
		WARNING("No instructions\n");
		exit(EXIT_FAILURE);
	}
	/* YOUR CODE GOES HERE */
    Instruction *instr;
    identifierList_t *id_list = idl_create();
    Instruction *tail;


    /* All READs and WRITEs are marked as critical
     * Get last instruction to define @tail */
    instr = head;
    while(instr != NULL) {
        int opcode = instr->opcode;
        if(opcode == WRITE || opcode == READ) {
            instr->critical = 1;
            if(opcode == WRITE) {
                id_add(id_list, instr->field1);
            }
        }
        if(instr->next == NULL) {
            tail = instr;
        }
        instr = instr->next;
    }

    /* All STORES relative to every write is marked critical */
    instr = head;
    while(instr != NULL) {
        if(instr->opcode == STORE && is_critical(id_list, instr)) {
            id_add(id_list, instr->field2);
            instr->critical = 1;
        }
        instr = instr->next;
    }

    /* Any instruction that influences the value of the
     * register pointed to by STORES is marked as critical.*/
    instr = tail;
    while(instr != NULL) {
        if(instr->critical) {
            instr = instr->prev;
            continue;
        }
        if(is_critical(id_list, instr)) {
            instr->critical = 1;
            switch(instr->opcode) {
                case ADD:
                case SUB:
                case MUL:
                case OR:
                case XOR:
                    id_add(id_list, instr->field3);
                case LOAD:
                case STORE:
                    id_add(id_list, instr->field2);
                    break;
                default:
                    break;
            }
        }
        instr = instr->prev;
    }

    /* Remove all instructions not marked as critical */
    instr = head;
    while(instr != NULL) {
        if(!instr->critical) {
            if(instr->prev == NULL) {
                head = instr->next;
                instr->prev = NULL;
                instr->next = NULL;
                free(instr);
                instr = head;
            } else {
                Instruction *next = instr->next;
                Instruction *prev = instr->prev;
                prev->next = next;
                next->prev = prev;
                free(instr);
                instr = next;
            }
            continue;
        }
        instr = instr->next;
    }

    /* Free dynamic allocations */
    idl_destroy(id_list);

	if (head) {
		PrintInstructionList(stdout, head);
		DestroyInstructionList(head);
	}
	return EXIT_SUCCESS;
}
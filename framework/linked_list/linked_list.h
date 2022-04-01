#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stdint.h>

typedef struct tagDoublyLinkedListNode{
    void *nodeData;
    struct tagDoublyLinkedListNode *prev;
    struct tagDoublyLinkedListNode *next;
} DoublyLinkedListNode;

int32_t InitDoublyLinkedListNode(DoublyLinkedListNode *node);
int32_t DeInitDoublyLinkedListNode(DoublyLinkedListNode *head);
DoublyLinkedListNode *CreateDoublyLinkedListNode(void *nodeData);
int32_t InsertHeadDoublyLinkedListNode(DoublyLinkedListNode *head, DoublyLinkedListNode *node);
int32_t InsertTailDoublyLinkedListNode(DoublyLinkedListNode *head, DoublyLinkedListNode *node);
int32_t DeleteHeadDoublyLinkedListNode(DoublyLinkedListNode *head, DoublyLinkedListNode *node);
int32_t DeleteTailDoublyLinkedListNode(DoublyLinkedListNode *head, DoublyLinkedListNode *node);
void SwapDoublyLinkedListNode(DoublyLinkedListNode *node1, DoublyLinkedListNode *node2);
void PrintDoublyLinkedListNode(DoublyLinkedListNode *head);

#endif
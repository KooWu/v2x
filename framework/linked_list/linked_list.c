#include "linked_list.h"
#include <stdlib.h>
#include "my_log.h"

int32_t InitDoublyLinkedListNode(DoublyLinkedListNode *node)
{
    if (NULL == node) {
        dbg("invalid param\n");
        return -1;
    }
    node->next = node;
    node->prev = node;
}

int32_t DeInitDoublyLinkedListNode(DoublyLinkedListNode *head)
{
    if (NULL == head) {
        dbg("invalid param\n");
        return -1;
    }
    DoublyLinkedListNode *delNode = NULL;
    DoublyLinkedListNode *node = head->next;
    for (; node != head; ) {
        delNode = node;
        node = node->next;
        if (NULL != delNode->nodeData) {
            free(delNode->nodeData);
        }
        free(delNode);
    }
    head->next = head;
    head->prev = head;
}

DoublyLinkedListNode *CreateDoublyLinkedListNode(void *nodeData)
{
    if (NULL == nodeData) {
        dbg("invalid param\n");
        return NULL;
    }

    DoublyLinkedListNode *node = (DoublyLinkedListNode *)malloc(sizeof(DoublyLinkedListNode));
    if (NULL == node) {
        dbg("malloc failed\n");
        return NULL;
    }
    node->nodeData = nodeData;
    (void)InitDoublyLinkedListNode(node);
    return node;
}

int32_t InsertHeadDoublyLinkedListNode(DoublyLinkedListNode *head, DoublyLinkedListNode *node)
{
    if ((NULL == head) || (NULL == node)) {
        dbg("invalid param\n");
        return -1;
    }
    DoublyLinkedListNode *headNext = head->next;
    node->next = headNext;
    node->prev = head;
    head->next = node;
    headNext->prev = node;
    return 0;
}

int32_t InsertTailDoublyLinkedListNode(DoublyLinkedListNode *head, DoublyLinkedListNode *node)
{
    if ((NULL == head) || (NULL == node)) {
        dbg("invalid param\n");
        return -1;
    }
    DoublyLinkedListNode *headPrev = head->prev;
    node->next = head;
    node->prev = headPrev;
    headPrev->next = node;
    head->prev = node;
    return 0;
}

int32_t DeleteHeadDoublyLinkedListNode(DoublyLinkedListNode *head, DoublyLinkedListNode *node)
{
    if ((NULL == head) || (NULL == node)) {
        dbg("invalid param\n");
        return -1;
    }
    DoublyLinkedListNode *headNext = head->next;
    if (headNext == head) {
        dbg("list is empty\n");
        return -1;
    }
    DoublyLinkedListNode *headNextNext = head->next->next;
    head->next = headNextNext;
    headNextNext->prev = head;
    node->nodeData = headNext->nodeData;
    free(headNext);
    return 0;
}

int32_t DeleteTailDoublyLinkedListNode(DoublyLinkedListNode *head, DoublyLinkedListNode *node)
{
    if ((NULL == head) || (NULL == node)) {
        dbg("invalid param\n");
        return -1;
    }
    DoublyLinkedListNode *headPrev = head->prev;
    if (headPrev == head) {
        dbg("list is empty\n");
        return -1;
    }
    DoublyLinkedListNode *headPrevPrev = head->prev->prev;
    head->prev = headPrevPrev;
    headPrevPrev->next = head;
    node->nodeData = headPrev->nodeData;
    free(headPrev);
    return 0;
}

static void SwapDoublyLinkedListNextNode(DoublyLinkedListNode *node1, DoublyLinkedListNode *node2)
{
    DoublyLinkedListNode *node1Prev = node1->prev;
    DoublyLinkedListNode *node2Next = node2->next;

    node1->next = node2Next;
    node2Next->prev = node1;

    node1Prev->next = node2;
    node2->next = node1;
    node1->prev = node2;
    node2->prev = node1Prev;
}

static void SwapDoublyLinkedListAdjacentNode(DoublyLinkedListNode *node1, DoublyLinkedListNode *node2)
{
    if (node1->next == node2) {
        SwapDoublyLinkedListNextNode(node1, node2);
    } else {
        SwapDoublyLinkedListNextNode(node2, node1);
    }
}

void SwapDoublyLinkedListNode(DoublyLinkedListNode *node1, DoublyLinkedListNode *node2)
{
    if ((node1->next == node2) || (node1->prev == node2)) {
        SwapDoublyLinkedListAdjacentNode(node1, node2);
        return;
    }

    DoublyLinkedListNode *node1Prev = node1->prev;
    DoublyLinkedListNode *node1Next = node1->next;
    DoublyLinkedListNode *node2Prev = node2->prev;
    DoublyLinkedListNode *node2Next = node2->next;

    node1Prev->next = node2;
    node2->next = node1Next;
    node1Next->prev = node2;
    node2->prev = node1Prev;

    node2Prev->next = node1;
    node1->next = node2Next;
    node2Next->prev = node2;
    node1->prev = node2Prev;
}

void PrintDoublyLinkedListNode(DoublyLinkedListNode *head)
{
    static int i = 0;
    
    DoublyLinkedListNode *curNode = head->next;
    for (; curNode != head; curNode = curNode->next) {
            i++;
            if (i == 22) {
        return;
    }
        if (NULL != curNode->nodeData) {
            printf("0x%p->", curNode->nodeData);
        }
    }
    printf("end\n");
}
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "linked_list.h"

typedef void (*ThreadPoolJobFunc)(void *arg);

typedef struct {
	ThreadPoolJobFunc jobFunc;
	void *arg;
} ThreadPoolJobInfo;


typedef struct {
    int val;
    ThreadPoolJobInfo job;
} TestLinkedListNode;

static void TestFunc(void *arg)
{
    printf("testfunc\n");
}

int32_t EnterTestLinkedList(void)
{
    int32_t arg = 100;
    DoublyLinkedListNode head;
    InitDoublyLinkedListNode(&head);

    TestLinkedListNode *nodeData1 = (TestLinkedListNode *)malloc(sizeof(TestLinkedListNode));
    if (nodeData1 == NULL) {
        printf("malloc failed\n");
        return -1;
    }
    printf("0x%p\n", nodeData1);
    nodeData1->job.jobFunc = TestFunc;
    nodeData1->job.arg = &arg;
    nodeData1->val = 1;
    DoublyLinkedListNode *node1 = CreateDoublyLinkedListNode(nodeData1);
    if (node1 == NULL) {
        printf("CreateDoublyLinkedListNode failed\n");
        return -1;
    }
    InsertHeadDoublyLinkedListNode(&head, node1);

    TestLinkedListNode *nodeData2 = (TestLinkedListNode *)malloc(sizeof(TestLinkedListNode));
    if (nodeData2 == NULL) {
        printf("malloc failed\n");
        return -1;
    }
    printf("0x%p\n", nodeData2);
    nodeData2->job.jobFunc = TestFunc;
    nodeData2->job.arg = &arg;
    nodeData2->val = 2;
    DoublyLinkedListNode *node2 = CreateDoublyLinkedListNode(nodeData2);
    if (node2 == NULL) {
        printf("CreateDoublyLinkedListNode failed\n");
        return -1;
    }
    InsertHeadDoublyLinkedListNode(&head, node2);

    TestLinkedListNode *nodeData3 = (TestLinkedListNode *)malloc(sizeof(TestLinkedListNode));
    if (nodeData3 == NULL) {
        printf("malloc failed\n");
        return -1;
    }
    printf("0x%p\n", nodeData3);
    nodeData3->job.jobFunc = TestFunc;
    nodeData3->job.arg = &arg;
    nodeData3->val = 3;
    DoublyLinkedListNode *node3 = CreateDoublyLinkedListNode(nodeData3);
    if (node3 == NULL) {
        printf("CreateDoublyLinkedListNode failed\n");
        return -1;
    }
    InsertHeadDoublyLinkedListNode(&head, node3);

    PrintDoublyLinkedListNode(&head);
    
    SwapDoublyLinkedListNode(node1, node3);
    //SwapDoublyLinkedListNode(node3, node1);
    PrintDoublyLinkedListNode(&head);


    TestLinkedListNode *nodeData4 = (TestLinkedListNode *)malloc(sizeof(TestLinkedListNode));
    if (nodeData4 == NULL) {
        printf("malloc failed\n");
        return -1;
    }
    printf("0x%p\n", nodeData4);
    nodeData4->job.jobFunc = TestFunc;
    nodeData4->job.arg = &arg;
    nodeData4->val = 4;
    DoublyLinkedListNode *node4 = CreateDoublyLinkedListNode(nodeData4);
    if (node4 == NULL) {
        printf("CreateDoublyLinkedListNode failed\n");
        return -1;
    }
    InsertHeadDoublyLinkedListNode(&head, node4);

    TestLinkedListNode *nodeData5 = (TestLinkedListNode *)malloc(sizeof(TestLinkedListNode));
    if (nodeData5 == NULL) {
        printf("malloc failed\n");
        return -1;
    }
    printf("0x%p\n", nodeData5);
    nodeData5->job.jobFunc = TestFunc;
    nodeData5->job.arg = &arg;
    nodeData5->val = 5;
    DoublyLinkedListNode *node5 = CreateDoublyLinkedListNode(nodeData5);
    if (node5 == NULL) {
        printf("CreateDoublyLinkedListNode failed\n");
        return -1;
    }
    
    InsertHeadDoublyLinkedListNode(&head, node5);
    PrintDoublyLinkedListNode(&head);
    #if 1
    DoublyLinkedListNode node6;
    SwapDoublyLinkedListNode(node3, node4);
    PrintDoublyLinkedListNode(&head);
    DeleteTailDoublyLinkedListNode(&head, &node6);
    //DeleteHeadDoublyLinkedListNode(&head, &node6);
    printf("delete 0x%p\n", node6.nodeData);
    free(node6.nodeData);
    #endif
    PrintDoublyLinkedListNode(&head);

    DeInitDoublyLinkedListNode(&head);
    return 0;
}

int main(void)
{
    printf("EnterTestLinkedList\n");
    EnterTestLinkedList();
    return 0;
}

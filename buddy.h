#ifndef _BUDDY_H_
#define _BUDDY_H_

#define MEM_MAX_SIZE 1024

typedef struct node
{
    int data;
    int start;
    int end;
    struct node *left;
    struct node *parent;
    struct node *right;
} node;

node *newNode(int data, node *parent, int start);
node *Allocate(int size);
void Deallocate(node *root);
void PreorderTraverse(node *root);

#endif
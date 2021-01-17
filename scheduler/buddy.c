
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <inttypes.h>
#include "buddy.h"

int numOfNode = 0;
node *treeRoot = NULL;


node *newNode(int data, node *parent, int start)
{
    node *node1 = (node *)malloc(sizeof(node));
    node1->data = data;
    node1->left = NULL;
    node1->right = NULL;
    node1->parent = parent;
    node1->start = start;
    node1->end = start + data - 1;
    return (node1);
}

node *alloc1(int size, int currentSize, node *grandParent, char dir, int start)
{
    node *parent;

    parent = newNode(currentSize, grandParent, start);

    if (grandParent != NULL)
    {
        if (dir == 'l')
            grandParent->left = parent;
        else
            grandParent->right = parent;
    }

    node *t1 = NULL;
    while (currentSize > size)
    {
        t1 = newNode(currentSize / 2, parent, start);

        parent->left = t1;
        parent = t1;

        currentSize /= 2;
    }

    if (t1 == NULL)
        return parent;
    return t1;
}

node *alloc2(node *root, int size)
{
    if (root == NULL)
        return NULL;

    //allocated place -> return NULL
    if (root->left == NULL && root->right == NULL)
        return NULL;

    if (root->data == 2 * size)
    {
        if (root->left == NULL)
        {
            node *t1 = newNode(size, root, root->start);

            root->left = t1;

            return t1;
        }
        else if (root->right == NULL)
        {
            node *t1 = newNode(size, root, root->start + root->data / 2);

            root->right = t1;

            return t1;
        }
        else
            return NULL; //there's no point to traverse down
    }
    else if (root->data > 2 * size)
    {
        if (root->left != NULL && root->right != NULL)
        {
            node *left = alloc2(root->left, size);
            if (left == NULL)
            {
                node *right = alloc2(root->right, size);
                return right;
            }
            else
                return left;
        }
        else if (root->left != NULL)
        {
            node *left = alloc2(root->left, size);
            if (left == NULL)
            {
                node *t1 = alloc1(size, root->data / 2, root, 'r', root->start + root->data / 2);

                return t1;
            }
            else
                return left;
        }
        else
        {
            node *right = alloc2(root->right, size);
            if (right == NULL)
            {
                node *t1 = alloc1(size, root->data / 2, root, 'l', root->start);

                return t1;
            }
            else
                return right;
        }
    }
    else
        return NULL;
}

node *Allocate(int size)
{
    int blockSize = (int)pow(2, ceil(log(size) / log(2)));

    if (treeRoot == NULL)
    {
        treeRoot = newNode(MEM_MAX_SIZE, NULL, 0);
        if (MEM_MAX_SIZE > blockSize)
            return alloc1(blockSize, MEM_MAX_SIZE / 2, treeRoot, 'l', 0);
        else
            return treeRoot;
    }
    else
        return alloc2(treeRoot, blockSize);
}

uint8_t DeallocateUtil(node *root)
{
    if (root == NULL)
        return 0;

    if (root->left == NULL && root->right == NULL)
    {
        node *parent = root->parent;
        if (parent != NULL)
        {
            if (parent->right == root)
                parent->right = NULL;
            else
                parent->left = NULL;
        }
        free(root);
        return DeallocateUtil(parent);
    }

    return 1;
}
void Deallocate(node *root)
{
    if (DeallocateUtil(root) == 0)
        treeRoot = NULL;
}

void PreorderTraverse(node *root)
{
    if (root == NULL)
        return;
    PreorderTraverse(root->left);
    PreorderTraverse(root->right);

    if (root->left == NULL && root->right == NULL)
        printf("block_size:%d start:%d end:%d\n", root->data, root->start, root->end);
}

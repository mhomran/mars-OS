/**
 * @file buddy.c
 * @author Mohamed Hassanin Mohamed
 * @brief This is a module for the buddy memory mangement system.
 * @version 0.1
 * @date 2021-01-10
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <inttypes.h>
#include "buddy.h"

int numOfNode = 0;
node *treeRoot = NULL;

/**
 * @brief Create a node give it's data
 * 
 * @param data the size of the memory the node represents.
 * @param parent a pointer to the parent.
 * @param start the start of the memory the node represents.
 * @return node* the created node
 */
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

/**
 * @brief This function allocates a piece of memory in the case of
 * there's no allocated memory in the system or there's no node with a size of
 * the double desired size in the memory tree.
 * 
 * @param size the desired memory size to allocate
 * @param currentSize the size to start from in the left branch tree
 *  (i.e. "128", 64, 32 if 32 is the desired and 128 is the currentSize in this case)  
 * @param grandParent a node to attach the root of the created tree to
 * @param dir the direction to attach the created tree to 'l' to attach it to the left of
 * the grand parent and 'r' for the right.
 * @param start the start of the piece of memory
 * @return node*a node pointer to the allocated piece of memory
 */
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

/**
 * @brief This function is called to allocate a piece of memory in the buddy system
 * when the tree is not empty (there's some memory allocated in it).
 * 
 * @param root the root to the tree
 * @param size the desired size to allocate
 * @return node* a node pointer to the allocated piece of memory
 */
node *alloc2(node *root, int size)
{
    if (root == NULL)
        return NULL;

    //allocated place -> return NULL
    if (root->left == NULL && root->right == NULL)
        return NULL;

    //if the this node has a size that equals to the double of the desired size
    if (root->data == 2 * size)
    {
        //If the left is free -> Allocate
        if (root->left == NULL)
        {
            node *t1 = newNode(size, root, root->start);

            root->left = t1;

            return t1;
        }
        //If the right is free -> Allocate
        else if (root->right == NULL)
        {
            node *t1 = newNode(size, root, root->start + root->data / 2);

            root->right = t1;

            return t1;
        }
        else
            //there's no point to traverse down
            return NULL; 
    }
    //if the this node has a size that is bigger than the double of the desired size
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
        //If the tree has left subtree -> solve(left subtree). If it finds a place, 
        // then the desired memory is allocated. Otherwise it call alloc1 for allocation.
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
        //If the tree has right subtree -> solve(right subtree). If it finds a place, 
        // then the desired memory is allocated. Otherwise it call alloc1 for allocation.
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

/**
 * @brief This function used to allocate a piece of memory in the buddy system 
 * memory mangement
 * 
 * @param size The desired memory size to allocate
 * @return node* a node pointer to the allocated piece of memory
 */
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

/**
 * @brief utility function to deallocate a piece of memory in the buddy system memory 
 * mangement system.
 * 
 * @param root a node to the node to deallocate
 * @return uint8_t 0 if the tree is empty, 1 otherwise 
 */
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

/**
 * @brief Deallocate a piece of memory from the buddy system memory mangement system.
 * 
 * @param root a node to the memory piece to deallocate.
 */
void Deallocate(node *root)
{
    if (DeallocateUtil(root) == 0)
        treeRoot = NULL;
}

/**
 * @brief traverse a binary tree in a preorder way to print its leaves that
 * represent the memory allocated.
 * 
 * @param root the root of the binary tree
 */
void PreorderTraverse(node *root)
{
    if (root == NULL)
        return;
    PreorderTraverse(root->left);
    PreorderTraverse(root->right);

    if (root->left == NULL && root->right == NULL)
        printf("block_size:%d start:%d end:%d\n", root->data, root->start, root->end);
}


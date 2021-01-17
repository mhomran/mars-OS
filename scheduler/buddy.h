/**
 * @file buddy.h
 * @author Mohamed Hassanin Mohamed
 * @brief This is a module for the buddy memory management system.
 * @version 0.1
 * @date 2021-01-10
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef _BUDDY_H_
#define _BUDDY_H_

#define MEM_MAX_SIZE 1024

/**
 * @brief A struct represents a node in a binary tree. It corresponds to
 * an allocated memory in the simulation if it's a leaf in the tree.
 * 
 */
typedef struct node
{
    int data; 			/**< the size of the node */
    int start;			/**< the start of the memory piece */
    int end;			/**< the end of the memory piece */
    struct node *left;		/**< a node representing the left subtree */
    struct node *parent;	/**< a node to the parent; equal NULL if there's no parent */
    struct node *right;		/**< a node representing the right subtree */
} node;

node *newNode(int data, node *parent, int start);
node *Allocate(int size);
void Deallocate(node *root);
void PreorderTraverse(node *root);

#endif /* _BUDDY_H_ */


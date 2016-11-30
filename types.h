#include <limits.h>
#ifndef TYPES_H
#define TYPES_H

typedef struct node node;

typedef struct nodePool nodePool;

struct node {
	char type;
	char *name;
	char *path;
	char *contents;
	node *parent;
	node *next;
	node *nextChild;
	nodePool *children;
	struct stat *node_attr;
};

struct nodePool {
	node *front;
	node *rear;
};

nodePool* create_nodePool();

void addCNode(nodePool *pool, node *n);

void addGNode(nodePool *pool, node *n);

void deleteGNode(nodePool *pool, node *n);

void deleteCNode(nodePool *pool, node *n);

#endif

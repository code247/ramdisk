#include "types.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

nodePool* create_nodePool(){
	nodePool *np = malloc(sizeof(nodePool));
	np->front = NULL;
	np->rear = NULL;
	return np;
}

void addCNode(nodePool *pool, node *n){
	if(pool->front == NULL && pool->rear == NULL){
		pool->front = n;
		pool->rear = n;
		n->nextChild = NULL;
	} else {
		pool->rear->nextChild = n;
		pool->rear = n;
		n->nextChild = NULL;
	}
}

void addGNode(nodePool *pool, node *n){
	if(pool->front == NULL && pool->rear == NULL){
		pool->front = n;
		pool->rear = n;
		n->next = NULL;
	} else {
		pool->rear->next = n;
		pool->rear = n;
		n->next = NULL;
	}
}

void deleteCNode(nodePool *pool, node *n){
	if (pool->front == NULL) return;
	node *p = NULL;
	node *t = pool->front;
	while (t != n){
		p = t;
		t = t->nextChild;
	}
	if(pool->front == t && pool->rear == t) {
		pool->front = NULL;
		pool->rear = NULL;
		t->nextChild = NULL;
	} else if (pool->front == t){
		pool->front = pool->front->nextChild;
		t->nextChild = NULL;
	} else if (pool->rear == t) {
		p->nextChild = NULL;
		pool->rear = p;
	} else {
		p->nextChild = t->nextChild;
		t->nextChild = NULL;
	}
}


void deleteGNode(nodePool *pool, node *n){
	if (pool->front == NULL) return;
	node *p = NULL;
	node *t = pool->front;
	while (t != n){
		p = t;
		t = t->next;
	}
	if(pool->front == t && pool->rear == t) {
		pool->front = NULL;
		pool->rear = NULL;
		t->next = NULL;
	} else if (pool->front == t){
		pool->front = pool->front->next;
		t->next = NULL;
	} else if (pool->rear == t) {
		p->next = NULL;
		pool->rear = p;
	} else {
		p->next = t->next;
		t->next = NULL;
	}
}

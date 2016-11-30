#define FUSE_USE_VERSION 26
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fuse.h>
#include <limits.h>
#include <dirent.h>
#include <libgen.h>
#include "types.h"

#define debug 0

nodePool *global;
node *root;
long int MAX_SIZE;

static void myfs_fullpath(char fpath[PATH_MAX], const char *path) {
	strcpy(fpath, root->path);
	if (strcmp(path, "/") != 0) strncat(fpath, path, PATH_MAX);
}

static node* return_node(const char* path){
	node* n = global->front;
	while (n != NULL){
		if (strcmp(path, n->path) == 0){
			return n;
		} else {
			n = n->next;
		}
	}
	return NULL;
}

void myfs_init(char *rootPath){
	root = (node *) malloc(sizeof(node));
	global = (nodePool *) malloc(sizeof(nodePool));
	root->children = create_nodePool();
	root->node_attr = (struct stat *) malloc(sizeof(struct stat));
	root->name = (char *) malloc(sizeof(char) * NAME_MAX);
	root->path = (char *) malloc(sizeof(char) * PATH_MAX);
	MAX_SIZE = MAX_SIZE - sizeof(node) - sizeof(struct stat) - sizeof(nodePool) - ( sizeof(char) * PATH_MAX) - ( sizeof(char) * NAME_MAX);
	strcpy(root->name, "/");
	strcpy(root->path, rootPath);
	root->parent = NULL;
	root->contents = NULL;
	root->next = NULL;
	root->nextChild = NULL;
	root->type = 'd';
	time_t curr;
	time(&curr);
	root->node_attr->st_atime = curr;
	root->node_attr->st_mtime = curr;
	root->node_attr->st_mode = 0755 | S_IFDIR;
	root->node_attr->st_ctime = curr;
	root->node_attr->st_nlink = 2;
	root->node_attr->st_gid = getgid();
	root->node_attr->st_uid = getuid();
	root->node_attr->st_size = sizeof(node) + sizeof(struct stat) + sizeof(nodePool) + sizeof(char) * PATH_MAX + sizeof(char) * NAME_MAX;
	addGNode(global, root);
	if (debug) printf("%s\n", root->path);
	return;
}

static int myfs_getattr(const char* path, struct stat* stbuf) {
	char fpath[PATH_MAX];
	myfs_fullpath(fpath, path);
	node* n = return_node(fpath);
	if (n == NULL) return -ENOENT;
	stbuf->st_ctime = n->node_attr->st_ctime;
	stbuf->st_atime = n->node_attr->st_atime;
	stbuf->st_mtime = n->node_attr->st_mtime;
	stbuf->st_mode = n->node_attr->st_mode;
	stbuf->st_nlink = n->node_attr->st_nlink;
	stbuf->st_uid = n->node_attr->st_uid;
	stbuf->st_gid = n->node_attr->st_gid;
	stbuf->st_size = n->node_attr->st_size;
	if (debug) printf("getattr: %s\n", fpath);
	return 0;
}

static int myfs_opendir(const char* path, struct fuse_file_info* fi){
	char fpath[PATH_MAX];
	myfs_fullpath(fpath, path);
	node *n = return_node(fpath);
	if (debug) printf("opendir: %s\n", fpath);
	if (n == NULL) return -ENOENT;
	return 0;
}

static int myfs_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi) {
	char fpath[PATH_MAX];
	myfs_fullpath(fpath, path);
	node *n = return_node(fpath);
	if (debug) printf("readdir: %s\n", fpath);
	if (n == NULL) return -ENOENT;
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	if (n->children->front == NULL) return 0;
	node *t = n->children->front;
	while (t != NULL){
		filler(buf, t->name, NULL, 0);
		t = t->nextChild;
	}
	return 0;
}

static int myfs_mkdir(const char* path, mode_t mode) {
	if(MAX_SIZE - sizeof(node) - sizeof(struct stat) - sizeof(struct nodePool) - ( sizeof(char) * PATH_MAX) - ( sizeof(char) * NAME_MAX) < 0)return -ENOSPC;
	char *dir_nm, *base_nm, *dir_path, *base_path;
	time_t curr;
	MAX_SIZE = MAX_SIZE - sizeof(node) - sizeof(struct stat) - sizeof(struct nodePool) - ( sizeof(char) * PATH_MAX) - ( sizeof(char) * NAME_MAX);
	char fpath[PATH_MAX];
	myfs_fullpath(fpath, path);
	dir_path = strdup(fpath);
	base_path = strdup(fpath);
	dir_nm = dirname(dir_path);
	base_nm = basename(base_path);
	node *parent_dir = return_node(dir_nm);
	if (parent_dir == NULL) return -ENOENT;
	node *n = (node *) malloc(sizeof(node));
	n->node_attr = (struct stat *)malloc(sizeof(struct stat));
	n->name = (char *) malloc(sizeof(char) * NAME_MAX);
	n->path = (char *) malloc(sizeof(char) * PATH_MAX);
	n->node_attr->st_mode = 0755 | S_IFDIR;
	n->node_attr->st_nlink = 2;
	n->node_attr->st_gid = getgid();
	n->node_attr->st_uid = getuid();
	n->node_attr->st_size = sizeof(node) + sizeof(struct stat) + sizeof(struct nodePool)+ sizeof(char) * PATH_MAX + sizeof(char) * NAME_MAX;
	time(&curr);
	n->node_attr->st_atime = curr;
	n->node_attr->st_ctime = curr;
	n->node_attr->st_mtime = curr;
	n->type = 'd';
	strcpy(n->name, base_nm);
	strcpy(n->path, fpath);
	n -> children = create_nodePool();
	n->contents = NULL;
	n->parent = parent_dir;
	addCNode(parent_dir->children, n);
	addGNode(global, n);
	return 0;
}

static int myfs_unlink(const char* path) {
	char fpath[PATH_MAX];
	myfs_fullpath(fpath, path);
	node* n = return_node(fpath);
	if (n == NULL) return -ENOENT;
	if (n->type == 'd') return -EISDIR;
	MAX_SIZE += n->node_attr->st_size;
	deleteCNode(n->parent->children, n);
	deleteGNode(global, n);
	free(n->name);
	free(n->path);
	free(n->contents);
	free(n->node_attr);
	free(n);
	return 0;
}

static int myfs_rmdir(const char* path) {
	char fpath[PATH_MAX];
	myfs_fullpath(fpath, path);
	node* n = return_node(fpath);
	if (n == NULL) return -ENOENT;
	if (n->children->front != NULL) return -ENOTEMPTY;
	deleteCNode(n->parent->children, n);
	if(debug) printf("rmdir: %s\n", fpath);
	deleteGNode(global, n);
	n->parent->node_attr->st_size += n->node_attr->st_size;
	MAX_SIZE += n->node_attr->st_size;
	free(n->name);
	free(n->path);
	free(n->children);
	free(n->node_attr);
	free(n);
	return 0;
}

static int myfs_create(const char* path, mode_t mode, struct fuse_file_info* fi){
	if(MAX_SIZE - sizeof(node) - sizeof(struct stat) - ( sizeof(char) * PATH_MAX) - ( sizeof(char) * NAME_MAX) < 0)return -ENOSPC;
	char *dir_nm, *base_nm, *dir_path, *base_path;
	time_t curr;
	time(&curr);
	MAX_SIZE -= MAX_SIZE - sizeof(node) - sizeof(struct stat) - ( sizeof(char) * PATH_MAX) - ( sizeof(char) * NAME_MAX);
	char fpath[PATH_MAX];
	myfs_fullpath(fpath, path);
	dir_path = strdup(fpath);
	base_path = strdup(fpath);
	dir_nm = dirname(dir_path);
	base_nm = basename(base_path);
	node *parent_dir = return_node(dir_nm);
	if (parent_dir == NULL) return -ENOENT;
	node *n = (node *) malloc(sizeof(node));
	n->node_attr = (struct stat *)malloc(sizeof(struct stat));
	n->children = NULL;
	n->name = (char *)malloc(sizeof(char) * NAME_MAX);
	n->path = (char *)malloc(sizeof(char) * PATH_MAX);
	n->parent = parent_dir;
	n->node_attr->st_mode = 0666 | S_IFREG;
	n->node_attr->st_nlink = 1;
	n->node_attr->st_gid = getgid();
	n->node_attr->st_uid = getuid();
	n->node_attr->st_size = 4096;
	n->node_attr->st_atime = curr;
	n->node_attr->st_ctime = curr;
	n->node_attr->st_mtime = curr;
	n->type = 'f';
	n->contents = NULL;
	strcpy(n->name, base_nm);
	strcpy(n->path, fpath);
	addCNode(parent_dir->children, n);
	addGNode(global, n);
	return 0;
}

static int myfs_open(const char* path, struct fuse_file_info* fi) {
	char fpath[PATH_MAX];
	myfs_fullpath(fpath, path);
	if (return_node(fpath) == NULL){
		return -ENOENT;
	} else {
		return 0;
	}
}

static int myfs_read(const char* path, char *buf, size_t size, off_t offset, struct fuse_file_info* fi) {
	time_t curr;
	char fpath[PATH_MAX];
	myfs_fullpath(fpath, path);
	node* n = return_node(fpath);
	if (n == NULL) return -ENOENT;
	if (n->type == 'd') return -EISDIR;
	if (offset < n->node_attr->st_size){
		if (offset + size > n->node_attr->st_size) size = n->node_attr->st_size - offset;
		memcpy(buf, n->contents + offset ,size);
		time(&curr);
		n->node_attr->st_atime = curr;
		return size;
	} else return 0;
}

static int myfs_write(const char* path, char *buf, size_t size, off_t offset, struct fuse_file_info* fi) {
	char fpath[PATH_MAX];
	myfs_fullpath(fpath, path);
	node* n = return_node(fpath);
	if (MAX_SIZE - size < 0) return -ENOSPC;
	if (n == NULL) return -ENOENT;
	if (n->type == 'd') return -EISDIR;
	time_t curr;
	if (size > 0){
		size_t s = n->node_attr->st_size;
		if (s == 0){
			offset = 0;
			n->contents = (char *) malloc(sizeof(char) * size);
			MAX_SIZE -= size;
		} else {
			if (offset > s) offset = s;
			char *new_contents = (char *)realloc(n->contents, sizeof(char) * (offset + size));
			n->contents = new_contents;
			MAX_SIZE = MAX_SIZE + s - offset - size;
		}
		time(&curr);
		memset(n->contents + offset, '\0', size);
		strncpy(n->contents + offset, buf, size);
		//memcpy(n->contents + offset, buf, size);
		n->node_attr->st_size = offset + size;
		n->node_attr->st_ctime = curr;
		n->node_attr->st_mtime = curr;
	}
	return 0;
}

static int myfs_truncate(const char* path, off_t size){
	char fpath[PATH_MAX];
	myfs_fullpath(fpath, path);
	node* n = return_node(fpath);
	if (n == NULL) return -ENOENT;
	return 0;
}

static int myfs_utimens(const char* path, const struct timespec ts[2]){
	char fpath[PATH_MAX];
	myfs_fullpath(fpath, path);
	node* n = return_node(fpath);
	if (n == NULL) return -ENOENT;
	return 0;
}

static struct fuse_operations myfs_oper = {
	.create = myfs_create,
	.open = myfs_open,
	.read = myfs_read,
	.mkdir = myfs_mkdir,
	.write = myfs_write,
	.opendir = myfs_opendir,
	.rmdir = myfs_rmdir,
	.readdir = myfs_readdir,
	.getattr = myfs_getattr,
	.unlink = myfs_unlink,
	.truncate = myfs_truncate,
	.utimens = myfs_utimens,
};

int main(int argc, char ** argv){
	if (argc >= 3){
		long int size;
		size = atoi(argv[argc - 1]);
		if (size > 0){
			MAX_SIZE  = size  * 1024 * 1024;
			myfs_init(argv[argc - 2]);
			return fuse_main(argc - 1, argv, &myfs_oper, NULL);
		} else {
			printf("Size of ramdisk should be greater than zero MB");
			exit(-1);
		}
	}
	else {
		printf("Two arguments required. Usage: ./ramdisk /path/to/rootdir <size in MB>\n");
		exit(-1);
	}
}

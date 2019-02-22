#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "mytar.h"
#define NAME LINKNAME 100
#define MODE UID GID CHKSUM DEVMAJOR DEVMINOR 8
#define SIZE MTIME 12
#define TYPEFLAG 1
#define MAGIC 6
#define UNAME GNAME 32
#define PREFIX 155

int main(int argc, char *argv[]) {
	DIR *d;
	int i, fd;
	int c_flag = 0;
	int t_flag = 0;
	int x_flag = 0;
	int v_flag = 0;
	int f_flag = 0;
	int S_flag = 0;
	int ctx = 0;
        if (argc < 2) {
                printf("%s: you must specify at least one of the 'ctx' "
                       "options.\n", argv[0]);
                printf("usage: %s [ctxSp[f tarfile]] [file1 [ file2 [...] ] ]"
                       "\n", argv[0]);
                return 1;
        }
	for (i = 0; i < strlen(argv[1]); i++) {
		if (argv[1][i] == 'c') {
			ctx++;
			if (!c_flag && !t_flag && !x_flag) {
				c_flag = 1;
			}
		}
		else if (argv[1][i] == 't') {
			ctx++;
			if (!c_flag && !t_flag && !x_flag) {
				t_flag = 1;
			}
		}
		else if (argv[1][i] == 'x') {
			ctx++;
			if (!c_flag && !t_flag && !x_flag) {
				x_flag = 1;
			}
		}
		else if (argv[1][i] == 'v') {
			v_flag = 1;
		}
		else if (argv[1][i] == 'f') {
			f_flag = 1;
		}
		else if (argv[1][i] == 'S') {
			S_flag = 1;
		}
		else {
			printf("%s: unrecognized option '%s'.\n",
				argv[0], argv[1]);
		}
	}	
	if (ctx > 1) {
		printf("%s: you must choose one of the 'ctx' options.\n",
                        argv[0]);
		printf("usage: %s [ctxSp[f tarfile]] [file1 [ file2 [...] ] ]"
                        "\n", argv[0]);
		return 1;
	}
	if (!f_flag) {
		printf("%s: you must specify the 'f' option.\n", argv[0]);
		return 1;
	}
	if (c_flag) {
		if (-1 == (fd = open(argv[2], O_CREAT | O_TRUNC))) {
			perror("open");
			exit(1);
		}
		if (!argv[3]) {
			close(fd);
			return 0;
		}
		if (NULL == (d = opendir(argv[3]))) {
			perror("opendir");
			exit(2);
		}
		carchive(d, fd);
	}
        return 0; 
}

/* creates an archive */
void carchive(DIR *dir, int fd) {
	DIR *d;
	struct dirent *ent;
	char wbuf[155] = { 0 };
	struct stat *buf = (struct stat*)malloc(sizeof(struct stat));
	while ((ent = readdir(dir))) {
		if (-1 == stat(ent->d_name, buf)) {
			perror("stat");
			exit(3);
		}
		if (S_ISDIR(buf->st_mode) && strcmp(ent->d_name, ".") &&
			strcmp(ent->d_name, "..")) {
			if (NULL == (d = opendir(ent->d_name))) {
				perror("opendir");
				exit(1);
			}
			carchive(d, fd);
		}
		else if (!S_ISDIR(buf->st_mode)) {
			strcpy(wbuf, ent->d_name);
			write();
		}
	}
}

void clear_arr(char *arr, int n) {
	int i;
	for (i = 0; i < n; i++) {
		arr[i] = 0;
	}
}

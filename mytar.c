#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <utime.h>
#include "mytar.h"
#define NAME 100
#define LINKNAME 100
#define MODE 8
#define UID 8
#define GID 8
#define CHKSUM 8
#define CHKSUM_OFF 148
#define DEVMAJOR 8
#define DEVMINOR 8
#define OCTAL 8
#define SIZE 12
#define MTIME 12
#define TYPEFLAG 1
#define MAGIC 6
#define VERSION 2
#define UNAME 32
#define GNAME 32
#define PREFIX 155
#define HDR 512
#define DISK 4096
#define PERMS 10

int v_flag = 0;
int S_flag = 0;

int main(int argc, char *argv[]) {
	int i, fd;
	struct stat *buf;
	int flag = 0;
	int f_flag = 0;
	int ctx = 0;
	char *path;
	char end[HDR * 2];
        if (argc < 2) {
                fprintf(stderr, "%s: you must specify at least one of the 'ctx'"
                                " options.\n", argv[0]);
                fprintf(stderr, "usage: %s [ctxSp[f tarfile]] "
				"[file1 [ file2 [...] ] ]\n", argv[0]);
                return 1;
        }
	for (i = 0; i < strlen(argv[1]); i++) {
		if (argv[1][i] == 'c') {
			ctx++;
			if (!flag) {
				flag = 'c';
			}
		}
		else if (argv[1][i] == 't') {
			ctx++;
			if (!flag) {
				flag = 't';
			}
		}
		else if (argv[1][i] == 'x') {
			ctx++;
			if (!flag) {
				flag = 'x';
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
			fprintf(stderr, "%s: unrecognized option '%c'.\n",
				argv[0], argv[1][i]);
		}
	}	
	if (ctx > 1) {
		fprintf(stderr, "%s: you may only choose one of"
                       " the 'ctx' options.\n", argv[0]);
		fprintf(stderr, "%s: //home/pn-cs357/demos/mytar "
			"[ctxSp[f tarfile]] [file1 [ file2 [...] ] ]\n",
			argv[0]);
		return 1;
	}
	if (!f_flag) {
		fprintf(stderr, "%s: you must specify the 'f' option.\n",
			argv[0]);
		return 1;
	}
	if (flag == 'c') {
		buf = (struct stat*)malloc(sizeof(struct stat));
		path = (char*)malloc(sizeof(char) * PATH_MAX);
		if (-1 == (fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC,
			S_IRUSR | S_IWUSR))) {
			perror(argv[2]);
			exit(1);
		}
		if (!argv[3]) {
			close(fd);
			return 0;
		}
		for (i = 3; i < argc; i++) {
			path[0] = '\0';
			if (-1 == lstat(argv[i], buf)) {
				/* do nothing */
			}
			else {
				strcat(path, argv[i]);
				if (path[strlen(path) - 1] != '/' &&
					S_ISDIR(buf->st_mode)) {
					strcat(path, "/");
				}
				if (S_ISDIR(buf->st_mode) &&
					1 == write_header(fd, path)) {
					path[strlen(path)-1] = '\0';
					octal_err(path);
					strcat(path, "/");
				}
				carchive(fd, path);
				/* closedir(d); */
			}
		}
		memset(end, 0, HDR * 2);
		write(fd, end, HDR * 2);
		close(fd);
		free(buf);
		free(path);
	}
	else {
		if (-1 == (fd = open(argv[2], O_RDONLY))) {
			perror("open");			
		}
		xtarchive(fd, argv, argc, flag);
	}
        return 0; 
}

/* creates an archive */
void carchive(int fd, char *path) {
	DIR *d;
	struct dirent *ent;
	struct stat *buf = (struct stat*)malloc(sizeof(struct stat));
	if (-1 == lstat(path, buf)) {
		perror(path);
		return;
	}
	if (S_ISDIR(buf->st_mode)) {
		if (NULL == (d = opendir(path))) {
			perror(path);
			return;
		}
		while ((ent = readdir(d))) {
			strcat(path, ent->d_name);
			if (-1 == lstat(path, buf)) {
				perror(ent->d_name);
				free(buf);
				return;
			}
			path[strlen(path) - strlen(ent->d_name)] = '\0';
			if (S_ISDIR(buf->st_mode) && strcmp(ent->d_name, ".") &&
				strcmp(ent->d_name, "..")) {
				strcat(path, ent->d_name);	
				strcat(path, "/");
				if (1 == write_header(fd, path)) {
					path[strlen(path)-1] = '\0';
					octal_err(path);
					strcat(path, "/");
				}
				carchive(fd, path);
				path[strlen(path)-strlen(ent->d_name)-1] = '\0';
			}
			else if (!S_ISDIR(buf->st_mode)) {
				strcat(path, ent->d_name);
				if (1 == write_header(fd, path)) {
					octal_err(path);
				}
				path[strlen(path) - strlen(ent->d_name)] = '\0';
			}
		}
		closedir(d);
	}
	else {		
		if (1 == write_header(fd, path)) {
			octal_err(path);
		}
	}
	free(buf);
}

/* listing an archive */
void xtarchive(int fd, char *argv[], int argc, char flag) {
	int fsize, up512, i, st_mode, chksum, fdout, num, wbytes = 0;
	char *usrgrp, *path;
	struct header *hdr = (struct header*)malloc(sizeof(struct header));
	struct utimbuf *times;
	time_t t, mtime;
	struct tm *time;
	char perms[PERMS] = "drwxrwxrwx";
	while (0 < read(fd, hdr, HDR)) {
		path = path_maker(hdr->prefix, hdr->name);
		if (!hdr->name[0]) {
			break;
		}
		fsize = strtol(hdr->size, NULL, OCTAL);
		if (argc < 4 || in(path, argv, argc)) {
			chksum = 0;
			for (i = 0; i < HDR; i++) {
				if (i < CHKSUM_OFF ||
					i >= CHKSUM_OFF + CHKSUM) {
					chksum += (unsigned char)
						((unsigned char*)(hdr))[i];
				}
				else {
					chksum += ' ';
				}
			}
			if (chksum != strtol(hdr->chksum, NULL, OCTAL)
				|| strncmp(hdr->magic, "ustar", MAGIC - 1) ||
				(S_flag && (strncmp(hdr->version, "00",
				VERSION) || strncmp(hdr->magic, "ustar\0",
				MAGIC) || (hdr->uid[0] < '0' ||
				hdr->uid[0] > '7')))) {
				fprintf(stderr, "Malformed header found.  "
					"Bailing.\n");
				exit(7);
			}
			if (flag == 't') {
				if (v_flag) {
					st_mode = strtol(hdr->mode, NULL, 8);
					for (i = 0; i < PERMS; i++) {
						if (!((st_mode << i) & 01000)) {
							perms[i] = '-';
						}
					}
					if (*(hdr->typeflag) == '5') {
						perms[0] = 'd';
					}
					else if (*(hdr->typeflag) == '2') {
						perms[0] = 'l';
					}
					else {
						perms[0] = '-';
					}
					t = strtol(hdr->mtime, NULL, OCTAL);
					time = localtime(&t);
					usrgrp = (char*)malloc(sizeof(char) *
						(strlen(hdr->uname) +
						strlen(hdr->gname) + 1));
					strcpy(usrgrp, hdr->uname);
					strcat(usrgrp, "/");
					strcat(usrgrp, hdr->gname);
					printf("%s %-17s %8ld %d-%02d-%02d"
					       " %02d:%02d ",
						perms, usrgrp,
						strtol(hdr->size, NULL, OCTAL),
						1900 + time->tm_year,
						1 + time->tm_mon,
						time->tm_mday, time->tm_hour,
						time->tm_min);
					strcpy(perms, "drwxrwxrwx");
					free(usrgrp);
				}
				if (hdr->prefix[0]) {
					strcat(hdr->prefix, "/");
				}
				printf("%.155s%.100s\n", hdr->prefix,
					hdr->name);
				hdr->prefix[strlen(hdr->prefix)] = '\0';
				if (fsize > 0) {
				 	up512 = (((fsize / HDR) + 1) * HDR);
					lseek(fd, up512, SEEK_CUR);
				}
			}
			else if (flag == 'x') {
				if ((fdout = restore_file(hdr)) && fsize) {
					wbytes = 0;
					while ((num = read(
						fd, hdr, HDR)) > 0 && wbytes <
						fsize) {
						wbytes += num;
						write(fdout, hdr,
							min(HDR, fsize));
						fsize -= HDR;
					}
					lseek(fd, -HDR, SEEK_CUR);
				}
				times = (struct utimbuf*)malloc(sizeof(
					struct utimbuf));
				mtime = (time_t)(strtol(
					hdr->mtime, NULL, OCTAL));
				times->actime = mtime;
				times->modtime = mtime;
				if (-1 == utime(path, times)) {
					perror(path);
					return;
				}
				if (v_flag) {
					printf("%.255s\n", path);
				}
				free(times);
				free(path);
			}
		}
	}
	free(hdr);
}

int min(int num1, int num2) {
	if (num1 < num2) {
		return num1;
	}
	else {
		return num2;
	}
}

int restore_file(struct header *hdr) {
	/* given a header file restore the file */
	int fd = 0;
	char *path;
	mode_t perms;
	perms = (mode_t)strtol(hdr->mode, NULL, OCTAL);
	path = path_maker(hdr->prefix, hdr->name);
	if (hdr->prefix[0]) {
		strcat(hdr->prefix, "/");
	}
	if (*(hdr->typeflag) == '5') {
		mkdir(path, perms);
	}
	else if (*(hdr->typeflag) == '2'){
		/* Create a symlink (figure out how to do that) */
		symlink(hdr->linkname, path);
	}
	else {
		if (-1 == (fd = creat(path, perms))) {
			perror(path);
			free(path);
			exit(1);
		}		
	}
	hdr->prefix[strlen(hdr->prefix)] = '\0';
	free(path);
	return fd;
}

int in(char *target, char *argv[], int argc) {
	int i;
	for (i = 3; i < argc; i++) {
		if (path_helper(argv[i], target)) {
			return 1;
		}
	}
	return 0;
}

int path_helper(char *prefix, char *path) {
	int i;
	for (i = 0; i < strlen(prefix); i++) {
		if (prefix[i] != path[i]) {
			return 0;
		}
	}
	return 1;
}

int write_header(int fdout, char *path) {
	/* Populate header struct and write to outfile. */
	int fdin, i;
	char *namestart;
	unsigned int chksum = 0;
	struct passwd *pwd;
	struct group *grp;
	struct stat *buf = (struct stat*)malloc(sizeof(struct stat));
	struct header *hdr;
	if (-1 == lstat(path, buf)) {
		perror(path);
		return 2;
	}
	hdr = (struct header*)malloc(sizeof(struct header));
	hdr = memset(hdr, 0, HDR);
	if (strlen(path) <= NAME) {
		strcpy(hdr->name, path);
	}
	else {
		strcpy(hdr->name, namestart = prefix_helper(path));
		strcpy(hdr->prefix, path);
		namestart[-1] = '/';
	}
	sprintf(hdr->mode, "%07o", buf->st_mode & 07777);
	if (buf->st_uid > 07777777) {
		if (S_flag) {
			free(buf);
			free(hdr);
			return 1;
		}
		insert_octal(hdr->uid, UID, buf->st_uid);
	}
	else {
		sprintf(hdr->uid, "%07o", buf->st_uid);
	}
	if (buf->st_gid > 07777777) {
		insert_octal(hdr->gid, GID, buf->st_uid);
	}
	else {
		sprintf(hdr->gid, "%07o", buf->st_gid);
	}
	if (S_ISLNK(buf->st_mode) || S_ISDIR(buf->st_mode)) {
		sprintf(hdr->size, "%011o", 0);
	}
	else if (buf->st_size > 077777777777) {
		insert_octal(hdr->size, SIZE, buf->st_size);
	}
	else {
		sprintf(hdr->size, "%011o", (int)buf->st_size);
	}
	if (buf->st_mtime > 077777777777) {
		insert_octal(hdr->mtime, MTIME, buf->st_mtime);
	}
	else {
		sprintf(hdr->mtime, "%07o", (int)buf->st_mtime);
	}
	if (S_ISLNK(buf->st_mode)) {
		*(hdr->typeflag) = '2';
		readlink(path, hdr->linkname, LINKNAME);
	}
	else if (S_ISDIR(buf->st_mode)) {
		*(hdr->typeflag) = '5';
	}
	else {
		*(hdr->typeflag) = '0';
	}
	strcpy(hdr->magic, "ustar");
	strcpy(hdr->version, "00");
	pwd = getpwuid(buf->st_uid);
	strcpy(hdr->uname, pwd->pw_name);
	grp = getgrgid(buf->st_gid);
	strcpy(hdr->gname, grp->gr_name);
	memset(hdr->chksum, ' ', CHKSUM);
	for (i = 0; i < HDR; i++) {
		chksum += (unsigned char)((unsigned char*)hdr)[i];
	}
	sprintf(hdr->chksum, "%07o", chksum);
	if (-1 == write(fdout, hdr, HDR)) {
		perror("write");
		return 1;
	}
	if (!S_ISDIR(buf->st_mode) && buf->st_size) {
		if (-1 == (fdin = open(path, O_RDONLY))) {
			perror(path);
			return 2;
		}
		write_content(fdin, fdout);
		close(fdin);
	}
	if (v_flag) {
		printf("%s\n", path);
	}
	free(buf);
	free(hdr);
	return 0;
}

char *path_maker(char *prefix, char *name) {
	char *path = (char*)calloc(strlen(prefix) + strlen(name) + 1,
		sizeof(char));
	if (prefix[0]) {
		strcpy(path, prefix);
		strcat(path, "/");
	}
	strcat(path, name);
	return path;
}

char *prefix_helper(char *path) {
	int i;
	char *name;
	char *endptr = path + strlen(path);
	for (;i < NAME; i++, endptr--) {
		if (*endptr == '/') {
			name = endptr;	
		}
	}
	*name = '\0';
	return name + 1;
}



void write_content(int fdin, int fdout) {
	/* Copy contents from file to .tar file.
	 * Write amount of '\0' to have total number of
	 * bytes rounded up to 512 bytes. */
	int num, bytes_written = 0;
	char buf[DISK];
	while ((num = read(fdin, buf, DISK)) > 0) {
		write(fdout, buf, num);
		bytes_written += num;
	}
	memset(buf, 0, DISK);
	write(fdout, buf, (((bytes_written / HDR) + 1) * HDR) - bytes_written);
}

void octal_err(char *path) {
	printf("(insert_octal:251) octal value too long. (013655557)\n"
	       "octal value too long. (013655557)\n"
	       "%s: Unable to create conforming header.  Skipping.\n", path);
}

int insert_octal(char *where, size_t size, int32_t val) {
	int err = 0;
	if (val < 0 || (size < sizeof(val))) {
		err++;
	}
	else {
		memset(where,0, size);
		*(int32_t *)(where+size-sizeof(val)) = htonl(val);
		*where |=0x80;
	}
	return err;
}

uint32_t extract_octal(char *where, int len) {
	int32_t val = -1;
	if ((len >= sizeof(val)) && (where[0] & 0x80)) {
		val = *(int32_t*)(where+len-sizeof(val));
		val = ntohl(val);
	}
	return val;
}

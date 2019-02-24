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
#include "mytar.h"
#define NAME 100
#define LINKNAME 100
#define MODE 8
#define UID 8
#define GID 8
#define CHKSUM 8
#define DEVMAJOR 8
#define DEVMINOR 8
#define OCTAL 8
#define SIZE 12
#define MTIME 12
#define TYPEFLAG 1
#define MAGIC 6
#define UNAME 32
#define GNAME 32
#define PREFIX 155
#define HDR 512
#define DISK 4096

int v_flag = 0;
int S_flag = 0;

int main(int argc, char *argv[]) {
	DIR *d;
	int i, fd;
	struct stat *buf;
	int c_flag = 0;
	int t_flag = 0;
	int x_flag = 0;
	int f_flag = 0;
	int ctx = 0;
	char *path;
	char end[HDR * 2];
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
		buf = (struct stat*)malloc(sizeof(struct stat));
		path = (char*)malloc(sizeof(char) * PATH_MAX);
		if (-1 == (fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC,
			S_IRUSR | S_IWUSR))) {
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
		if (-1 == lstat(argv[3], buf)) {
			perror("stat");
			exit(3);
		}
		strcat(path, argv[3]);
		strcat(path, "/");
		if (write_header(fd, path)) {
			octal_err(path);
		}
		carchive(d, fd, path);
		memset(end, 0, HDR * 2);
		write(fd, end, HDR * 2);
		closedir(d);
		free(buf);
		free(path);
	}
        return 0; 
}

/* creates an archive */
void carchive(DIR *dir, int fd, char *path) {
	DIR *d;
	struct dirent *ent;
	struct stat *buf = (struct stat*)malloc(sizeof(struct stat));
	while ((ent = readdir(dir))) {
		strcat(path, ent->d_name);
		if (-1 == lstat(path, buf)) {
			perror("stat");
			free(buf);
			exit(3);
		}
		path[strlen(path) - strlen(ent->d_name)] = '\0';
		if (S_ISDIR(buf->st_mode) && strcmp(ent->d_name, ".") &&
			strcmp(ent->d_name, "..")) {
			strcat(path, ent->d_name);
			strcat(path, "/");
			if (write_header(fd, path)) {
				octal_err(path);
			}
			if (NULL == (d = opendir(path))) {
				perror("opendir");
				free(buf);
				exit(2);
			}
			carchive(d, fd, path);
			closedir(d);
			path[strlen(path) - strlen(ent->d_name) - 1] = '\0';
		}
		else if (!S_ISDIR(buf->st_mode)) {
			strcat(path, ent->d_name);
			if (write_header(fd, path)) {
				octal_err(path);
			}
			}
	}
	free(buf);
}

int write_header(int fdout, const char *path) {
	/* Populate header struct and write to outfile. */
	int fdin;
	struct passwd *pwd;
	struct group *grp;
	struct stat *buf = (struct stat*)malloc(sizeof(struct stat));
	struct header *hdr; 
	if (-1 == lstat(path, buf)) {
		printf("%s\n", path);
		perror("stat");
		free(buf);
		exit(3);
	}
	hdr = (struct header*)malloc(sizeof(struct header));
	hdr = memset(hdr, 0, HDR);
	strcpy(hdr->name, path);
	sprintf(hdr->mode, "%07o", buf->st_mode);
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
	if (buf->st_size > 0777777) {
		insert_octal(hdr->size, SIZE, buf->st_size);
	}
	else {
		sprintf(hdr->size, "%07o", (int)buf->st_size);
	}
	if (buf->st_mtime > 0777777) {
		insert_octal(hdr->mtime, MTIME, buf->st_mtime);
	}
	else {
		sprintf(hdr->mtime, "%07o", (int)buf->st_mtime);
	}
	if (S_ISLNK(buf->st_mode)) {
		*(hdr->typeflag) = '2';
		readlink(path, hdr->linkname, 100);
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
	if (major(buf->st_dev) > 07777777) { 
		insert_octal(hdr->devmajor, DEVMAJOR, major(buf->st_dev));
	}
	else {
		sprintf(hdr->devmajor, "%07o", major(buf->st_dev));
	}
	if (minor(buf->st_dev) > 07777777) {
		insert_octal(hdr->devminor, DEVMINOR, minor(buf->st_dev));
	}
	else {
		sprintf(hdr->devminor, "%07o", minor(buf->st_dev));
	}
	if (-1 == write(fdout, hdr, HDR)) {
		perror("write");
		exit(4);
	}
	if (v_flag) {
		printf("%s\n", path);
	}
	if (!S_ISDIR(buf->st_mode)) {
		if (-1 == (fdin = open(path, O_RDONLY))) {
			perror("open");
			exit(1);
		}
		write_content(fdin, fdout);
	}
	free(buf);
	free(hdr);
	return 0;
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

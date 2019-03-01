#ifndef MYTAR_H
#define MYTAR_H

struct header {
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char chksum[8];
	char typeflag[1];
	char linkname[100];
	char magic[6];
	char version[2];
	char uname[32];
	char gname[32];
	char devmajor[8];
	char devminor[8];
	char prefix[155];
	char padding[12];	
};

void carchive(int, char*);

void xtarchive(int, char *argv[], int, char);

int restore_file(struct header*);

char *dotdot(char*);

char *path_maker(char*, char*);

int min(int, int);

char *prefix_helper(char*);

char *parent_dir(char*);

int path_helper(char*, char*);

int tin(char*, char *argv[], int);

int xin(char*, char, char *argv[], int);

int write_header(int, char*);

void write_content(int, int);

int insert_octal(char*, size_t, int32_t);

uint32_t extract_octal(char*, int);

void octal_err(char*);

#endif

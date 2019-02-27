#ifndef MYTAR_H
#define MYTAR_H

void carchive(int, char*);

void xtarchive(int, char *argv[], int, char);

char *prefix_helper(char*);

int path_helper(char*, char*);

int in(char*, char *argv[], int);

int write_header(int, char*);

void write_content(int, int);

int insert_octal(char*, size_t, int32_t);

uint32_t extract_octal(char*, int);

void octal_err(char*);

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

#endif

#ifndef MYTAR_H
#define MYTAR_H

void carchive(DIR*, int, char*);

void write_header(int, const char*);

void write_content(int, int);

int insert_special_int(char*, size_t, int32_t);

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

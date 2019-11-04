#ifndef FSDATA_CUSTOM_H
#define FSDATA_CUSTOM_H

#include "fsdata.h"

struct dn_file {
	char *data;
	int len;
	int coreLen;
	int index;
	void *pextension;
	u8_t flags;
	u8_t is_custom_file;
};

struct dn_data {
	float temp;
	float tempHist[100];
	unsigned int tempLen;
};

int fs_open_custom(struct fs_file *file, const char *name);
void fs_close_custom(struct fs_file *file);
void fsdata_updateData(unsigned int id, uint32_t data);
void fsdata_initDataStruct(void);
void findMinMax(float *data, unsigned int dataLen, float *min, float *max);

#endif
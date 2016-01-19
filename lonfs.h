

typedef struct
{
	char *path;
	char *url;
	size_t size;
	mode_t mode;
	int8_t *binfo;
} _file_t;

typedef struct
{
	size_t count;
	_file_t **file; /* probabily will be implemented with The Opencall
		Array Manager */
} _filelist_t;

#define MAX_FILES 16

inline int8_t
get_binfo(int fid);



#include "lib.h"



void out(char *name, int isdir, int isLast, int level);
void walk_file(char *path, char *name, int isLast, int level);

void walk_dir(char *path, int level)
{
	int fd, n;
	struct File f;

	if ((fd = open(path, O_RDONLY)) < 0)
		user_panic("open %s: %e", path, fd);

	while ((n = readn(fd, &f, sizeof f)) == sizeof f)
		if (f.f_name[0]) {
            char buf[MAXPATHLEN];
            strcpy(buf, path); strlcat(buf, "/", MAXPATHLEN); strlcat(buf, f.f_name, MAXPATHLEN);
            walk_file(buf, f.f_name, 0, level + 1);
        }
			
	if (n > 0) {
		writef("File Block Damaged!\n");
		exit(-1);
	}
	if (n < 0) {
		writef("Failed To Read File Block!\n");
		exit(-1);
	}
}

void walk_file(char *path, char *name, int isLast, int level)
{
	int r;
	struct Stat st;

	if ((r=stat(path, &st)) < 0){
		writef("Directory/File Doesn't Exist!\n");
		exit(-1);
	}
    out(name, st.st_isdir, isLast, level);
	if (st.st_isdir)
		walk_dir(path, level);
		
}

void out(char *name, int isdir, int islast, int level) {
    if (level >= 0) {
        while (level--) {
            fwritef(1, "|   ");
        }
        if (islast) fwritef(1, "`-- ");

        else fwritef(1, "|-- ");
    }
    
    if(isdir) fwritef(1, B_BLUE);
    fwritef(1, "%s"C_RESET"\n", name);
}


void
umain(int argc, char **argv)
{
	int i;
    //for (i = 0; i < argc; i++) writef("%s ", argv[i]);
	if (argc == 1)
		walk_file("/", ".", 0, -1);
	else if (argc >= 2) {
		walk_file(argv[1], argv[1], 0, -1);
	}
}



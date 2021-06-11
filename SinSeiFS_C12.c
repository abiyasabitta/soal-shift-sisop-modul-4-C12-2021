#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>


static const char *dirpath = "/home/abit/Downloads";
static const char *AtoZLogPath = "/home/abit/AtoZ.log";
static const char *FSLogPath = "/home/abit/SinSeiFS.log";

void Atbash(char *name, int isFile)
{
    int change = 1;
    for(int i = 0; i<strlen(name); i++)
    {
        if(name[i] == '.' && isFile) change=0;
        if(change)
        {
            if(name[i]>='A'&&name[i]<='Z') name[i] = 'Z'+'A'-name[i];
            if(name[i]>='a'&&name[i]<='z') name[i] =  'z'+'a'-name[i];
        }
    }
}

void fsLog(char *level, char *cmd,int descLen, const char *desc[])
{
    FILE *f = fopen(FSLogPath, "a");
    time_t now;
	time ( &now );
	struct tm * timeinfo = localtime (&now);
	fprintf(f, "%s::%s::%02d%02d%04d-%02d:%02d:%02d",level,cmd,timeinfo->tm_mday,timeinfo->tm_mon+1,timeinfo->tm_year+1900,timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    for(int i = 0; i<descLen; i++)
    {
        fprintf(f,"::%s",desc[i]);
    }
    fprintf(f,"\n");
    fclose(f);
}


void AtoZLog(char *cmd, char *firstPath, char *secondPath)
{
    FILE *f = fopen(AtoZLogPath, "a");

    if(!strcmp(cmd,"RENAME"))
    {
        fprintf(f, "%s::%s -> %s\n", cmd, firstPath, secondPath);
    }
    else if(!strcmp(cmd,"MKDIR"))
    {
        fprintf(f, "%s::%s\n", cmd, firstPath);
    }

    fclose(f);
}


int getfpath(const char *path, char *fpath, int isFile)
{
    int change = 0;
    if(strcmp(path,"/") == 0)
    {
        path=dirpath;

        sprintf(fpath,"%s",path);
    }
    else
    {
        strcat(fpath,dirpath);
        char pathCopy[1024] = {0};
        strcpy(pathCopy,path);

        char *lastPos = strrchr(path,'/');
        *lastPos++;
        const char *p="/";
        char *a,*b;
        for( a=strtok_r(pathCopy,p,&b) ; a!=NULL ; a=strtok_r(NULL,p,&b) ) {
            if(!change)
            {
                strcat(fpath,"/");
                strcat(fpath,a);
            }
            else
            {
                char changeName[1024] = {0};
                strcpy(changeName,a);
                if(!strcmp(changeName,lastPos) && isFile) Atbash(changeName,1);
                else Atbash(changeName,0);
                strcat(fpath,"/");
                strcat(fpath,changeName);
            }
            if(strncmp(a,"AtoZ_",5) == 0) change = 1;
            //printf("a = %s change = %d\n",a,change);
        }
    }

    return change;
    printf("getfpath fpath = %s\n",fpath);
}


static  int  xmp_getattr(const char *path, struct stat *stbuf)
{
    int res;
    char fpath[1024] = {0};
    getfpath(path,fpath,1);
    if(access(fpath,F_OK) == -1)
    {
        printf("gagal access fpath = %s\n", fpath);
        memset(fpath,0,sizeof(fpath));
        getfpath(path,fpath,0);
    }
    printf("getattr path = %s\nfpath = %s\n",path,fpath);

    res = lstat(fpath, stbuf);
    
    const char *desc[] = {path};
    fsLog("INFO","GETATTR",1, desc);
    if (res == -1) return -errno;

    return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    char fpath[1024] = {0};
    getfpath(path,fpath,1);

    int res = 0;
    int fd = 0 ;

    (void) fi;

    fd = open(fpath, O_RDONLY);

    if (fd == -1) return -errno;

    res = pread(fd, buf, size, offset);

    if (res == -1) res = -errno;

    close(fd);
    printf("readdir path = %s\nfpath = %s\n",path,fpath);
    const char *desc[] = {path};
    fsLog("INFO","READ",1, desc);
    return res;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    char fpath[1024] = {0};
    int change = 0;
    change = getfpath(path,fpath,0);
    int res = 0;
    printf("readdir path = %s\nfpath = %s\n",path,fpath);
    DIR *dp;
    struct dirent *de;
    (void) offset;
    (void) fi;

    dp = opendir(fpath);

    if (dp == NULL) return -errno;

    while ((de = readdir(dp)) != NULL) {
        struct stat st;

        memset(&st, 0, sizeof(st));

        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if(change)
        {
            char nameCopy[1024] = {0};
            strcpy(nameCopy,de->d_name);
            if(S_ISDIR(st.st_mode))
            {
                Atbash(nameCopy,0);
            }
            else Atbash(nameCopy,1);
            res = (filler(buf, nameCopy, &st, 0));
            //printf("readdir nameCopy = %s\n",nameCopy);
        }
        else{
            res = (filler(buf, de->d_name, &st, 0));
            //printf("readdir de->d_name = %s\n",de->d_name);
        }
        if(res!=0) break;
    }

    closedir(dp);
    const char *desc[] = {path};
    fsLog("INFO","READDIR",1, desc);
    return 0;
}

static int xmp_rename(const char *from, const char *to)
{
    char fullFrom[1024],fullTo[1024];
    if(strcmp(from,"/") == 0)
	{
		from=dirpath;
		sprintf(fullFrom,"%s",from);
	}
	else sprintf(fullFrom, "%s%s",dirpath,from);

    if(strcmp(to,"/") == 0)
	{
		to=dirpath;
		sprintf(fullTo,"%s",to);
	}
	else sprintf(fullTo, "%s%s",dirpath,to);

	int res;
    printf("rename from = %s to = %s\n",fullFrom, fullTo);
	res = rename(fullFrom, fullTo);
    DIR *dp = opendir(fullTo);
    char *lastSlash = strrchr(fullTo,'/');
    char *toAtoZ_ = strstr(lastSlash-1,"/AtoZ_");


    if(toAtoZ_ != NULL && dp != NULL) 
    {
        printf("a dir renamed to /AtoZ_\n");
        closedir(dp);
        AtoZLog("RENAME",fullFrom,fullTo);
    }
	if (res == -1)
		return -errno;
    
    const char *desc[] = {from,to};
    fsLog("INFO","RENAME",2, desc);
	return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
    char fpath[1024] = {0};
    getfpath(path,fpath,0);

    int res;
      char *lastSlash = strrchr(fpath,'/');
      char *toAtoZ_ = strstr(lastSlash-1,"/AtoZ_");

    res = mkdir(fpath, mode);
      if(toAtoZ_ != NULL) AtoZLog("MKDIR",fpath, "");
    if (res == -1)
      return -errno;

      const char *desc[] = {path};
      fsLog("INFO","MKDIR",1, desc);
    return 0;
}

static int xmp_rmdir(const char *path)
{
    char fpath[1024] = {0};
    getfpath(path,fpath,0);
    int res;

    res = rmdir(fpath);
    if (res == -1)
      return -errno;

      const char *desc[] = {path};
      fsLog("WARNING","RMDIR",1, desc);
    return 0;
}

static int xmp_unlink(const char *path)
{
    char fpath[1024] = {0};
    getfpath(path,fpath,1);

    int res;

    res = unlink(fpath);
    if (res == -1)
      return -errno;
      const char *desc[] = {path};
      fsLog("WARNING","UNLINK",1, desc);
    return 0;
}

static int xmp_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
    char fpath[1024] = {0};
    getfpath(path,fpath,1);

    int fd;
    int res;

    (void) fi;
    fd = open(fpath, O_WRONLY);
    if (fd == -1)
      return -errno;

    res = pwrite(fd, buf, size, offset);
    if (res == -1)
      res = -errno;

    close(fd);
      printf("write path = %s\nfpath = %s\n",path,fpath);
      const char *desc[] = {path};
      fsLog("INFO","WRITE",1, desc);
    return res;
}

static int xmp_truncate(const char *path, off_t size)
{
    char fpath[1024] = {0};
    getfpath(path,fpath,1);
    int res;

    res = truncate(fpath, size);
    if (res == -1)
      return -errno;
      printf("truncate path = %s\nfpath = %s\n",path,fpath);
      const char *desc[] = {path};
      fsLog("INFO","TRUNCATE",1, desc);
    return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
    char fpath[1024] = {0};
    getfpath(path,fpath,1);
    int res;

    res = open(fpath, fi->flags);
    if (res == -1)
        return -errno;
    const char *desc[] = {path};
    fsLog("INFO","OPEN",1, desc);
    close(res);

    printf("open path = %s\nfpath = %s\n",path,fpath);
    return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
    char fpath[1024] = {0};
    //getfpath(path,fpath,1);
    if(strcmp(path,"/") == 0)
    {
      path=dirpath;
      sprintf(fpath,"%s",path);
    }
    else sprintf(fpath, "%s%s",dirpath,path);
    int res;

      printf("mknod fpath = %s\n",fpath);
    /* On Linux this could just be 'mknod(path, mode, rdev)' but this
       is more portable */
    if (S_ISREG(mode)) {
      res = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
      if (res >= 0)
        res = close(res);
    } else if (S_ISFIFO(mode))
      res = mkfifo(fpath, mode);
    else
      res = mknod(fpath, mode, rdev);
    if (res == -1)
      return -errno;

      printf("mknod path = %s\nfpath = %s\n",path,fpath);
      const char *desc[] = {path};
      fsLog("INFO","MKNOD",1, desc);
    return 0;
}

static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .read = xmp_read,
    .rename = xmp_rename,
    .mkdir = xmp_mkdir,
    .rmdir = xmp_rmdir,
    .unlink = xmp_unlink,
    .write = xmp_write,
    .truncate = xmp_truncate,
    .open = xmp_open,
    .mknod = xmp_mknod,
};



int  main(int  argc, char *argv[])
{
    umask(0);

    return fuse_main(argc, argv, &xmp_oper, NULL);
}

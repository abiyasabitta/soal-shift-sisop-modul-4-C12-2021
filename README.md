# soal-shift-sisop-modul-4-C12-2021

### Soal 1
Untuk soal 1 diminta untuk membuat file system dimana memirrorkan direktori Downloads. Selain itu diminta untuk mengencode direktori yang bernama AtoZ_ menggunakan Atbash dimana A menjadi Z, B menjadi Y, dan seterusnya. Untuk itu digunakan FUSE untuk membuat file system yang memirror Downloads.
```cpp
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
```
Untuk nomer 1 menggunakan fungsi getfpath dimana akan menyimpan direktori asal dan akan mengecek apabila perlu di encode atau tidak
```cpp
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
        }
        else{
            res = (filler(buf, de->d_name, &st, 0));
        }
        if(res!=0) break;
    }

    closedir(dp);
    const char *desc[] = {path};
    fsLog("INFO","READDIR",1, desc);
    return 0;
}
```
Kemudian untuk mengencode digunakan fungsi Atbash
```cpp
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
```


### Soal 4
Untuk soal 4 membuat log sehingga membuat fungsi fslog dimana akan membuat log
```cpp
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
```
dimana fungsi tersebut dijalankan setiap operasi selesai dijalankan

### Kendala
Untuk kendala tidak bisa mengerjakan nomer 2 dan 3 dan juga untuk log nomer 1 masih belum bisa dijalankan.

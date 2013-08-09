int      another        (int *pargc, char ***pargv, char *prompt);
void     changetype     (int newtype, int show);
void     setbinary      (int argc, char **argv);
void     setascii       (int argc, char **argv);
void     settenex       (int argc, char **argv);
int      getit          (int argc, char **argv, int restartit, char *mode);
char     *remglob       (char **argv, int doswitch);
char     *onoff         (int bool);
void     setbell        (int argc, char **argv);
void     settrace       (int argc, char **argv);
void     sethash        (int argc, char **argv);
void     setverbose     (int argc, char **argv);
void     setpassive     (int argc, char **argv);
void     setport        (int argc, char **argv);
void     setprompt      (int argc, char **argv);
void     setglob        (int argc, char **argv);
void     mabort         (int sig, int code);
void     pwd            (int argc, char **argv);
void     quote1         (char *initial, int argc, char **argv);
void     quit           (int argc, char **argv);
void     disconnect     (int argc, char **argv);
int      confirm        (char *cmd, char *file);
void     fatal          (char *msg);
int      globulize      (char *cpp[]);
void     proxabort      (int sig, int code);
void     setcase        (int argc, char **argv);
void     setcr          (int argc, char **argv);
char     *dotrans       (char *name);
char     *domap         (char *name);
void     setsunique     (int argc, char **argv);
void     setrunique     (int argc, char **argv);
void     cdup           (int argc, char **argv);
void     syst           (int argc, char **argv);

void     ftpsetdebug    (int argc, char **argv);
void     setpeer        (int argc, char **argv);
void     settype        (int argc, char **argv);
void     ftpsetmode     (int argc, char **argv);
void     setform        (int argc, char **argv);
void     setstruct      (int argc, char **argv);
void     put            (int argc, char **argv);
void     mput           (int argc, char **argv);
void     reget          (int argc, char **argv);
void     get            (int argc, char **argv);
void     mget           (int argc, char **argv);
void     status         (int argc, char **argv);
void     cd             (int argc, char **argv);
void     lcd            (int argc, char **argv);
void     lpwd           (int argc, char **argv);
void     delete         (int argc, char **argv);
void     mdelete        (int argc, char **argv);
void     renamefile     (int argc, char **argv);
void     ls             (int argc, char **argv);
void     mls            (int argc, char **argv);
void     shell          (int argc, char **argv);
void     user           (int argc, char **argv);
void     makedir        (int argc, char **argv);
void     removedir      (int argc, char **argv);
void     quote          (int argc, char **argv);
void     site           (int argc, char **argv);
void     do_chmod       (int argc, char **argv);
void     do_umask       (int argc, char **argv);
void     idle           (int argc, char **argv);
void     rmthelp        (int argc, char **argv);
void     account        (int argc, char **argv);
void     doproxy        (int argc, char **argv);
void     setntrans      (int argc, char **argv);
void     setnmap        (int argc, char **argv);
void     restart        (int argc, char **argv);
void     macdef         (int argc, char **argv);
void     sizecmd        (int argc, char **argv);
void     modtime        (int argc, char **argv);
void     rmtstatus      (int argc, char **argv);
void     newer          (int argc, char **argv);
void     reset          (int argc, char **argv);
                                                 
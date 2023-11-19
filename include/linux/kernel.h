int tty_write(unsigned ch,char * buf,int count);

#define suser() (current->euid == 0)


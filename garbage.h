#ifndef __GARBAGE__H
#define __GARBAGE__H


void garbage_init(void);
void garbage_final(void);
char *garbage_category(char *category);

//为什么能用 127.0.0.1  我们是vscode 远程连接到香橙派 --  实际上是在orangepi上运行，用本地ip即可
#define WGET_CMD "wget http://127.0.0.1:8080/?action=snapshot -O/tmp/garbage.jpg"
#define GARBAGE_FILE "/tmp/garbage.jpg"

#endif

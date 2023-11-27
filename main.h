
#ifndef EXT2_MAIN_H
#define EXT2_MAIN_H


extern char current_path[256];
extern char current_user[10];

extern void initialize_user();  //初始化用户
extern int login(char username[10], char password[10]); //用户登录
extern void initialize_memory(); //初始化内存
extern void format(); //格式化文件系统
extern void cd(char tmp[100]); //进入某个目录，实际上是改变当前路径
extern void mkdir(char tmp[100], int type); //创建目录
extern void create(char tmp[100], int type); //创建文件
extern void rmdir(char tmp[100]); //删除一个空目录
extern void del(char tmp[100]); //删除文件
extern void open_file(char tmp[100]); //打开文件
extern void close_file(char tmp[100]); //关闭文件
extern void read_file(char tmp[100]); //读文件内容
extern int write_file(char tmp[100]); //文件以覆盖方式写入
extern void ls(); //查看目录下的内容
extern void check_disk(); //检查磁盘状态
extern void help(); //查看指令
extern void chmod(char tmp[100], unsigned short mode); //修改文件权限
extern void mkusr(); //创建用户
extern void rmusr(); //删除用户
extern void chusrname(); //修改用户名
extern void chusrpwd(); //修改用户密码

#endif //EXT2_MAIN_H

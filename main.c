#include <stdio.h>
#include <string.h>
#include "main.h"
#include "lib.h"

int main() {
    while (1)
    {
        int chusr_flag = 0;
        char command[10], temp[100];
        char username[10], password[10];
        user_index = -1;
        user_num = 0;
        initialize_user();
        while (1) {
            printf("user name: ");
            scanf("%s", username);
            if (!strcmp(username, "quit"))
                return 0;
            printf("password: ");
            scanf("%s", password);
            if (login(username, password)) {
                strcpy(current_disk, User[user_index].disk_name);
                strcpy(current_user, username);
                strcpy(current_path, "[");
                strcat(current_path, current_user);
                strcat(current_path, "@ext2 /");
                printf("User %s sign in!\n", username);
                break;
            }
            else {
                printf("User name or password wrong, please enter again!\n");
                printf("If want to exit, please enter \"quit\"!\n");
            }
        }
        while(getchar() != '\n');
        initialize_memory();
        check_disk();
        while (1) {
            int write_ret = -1;
            printf("%s]#", current_path);
            char c = getchar();
            if (c == '\n')
                continue;
            scanf("%s", command);
            // 将c放到command中，因为command中
            int len = strlen(command);
            for (int i = len; i >= 0; i--) {
                command[i + 1] = command[i];
            }
            command[0] = c;
            // printf("command: %s\n", command);
            if (!strcmp(command, "cd")) { //进入当前目录下
                scanf("%s", temp);
                cd(temp);
            }
            else if (!strcmp(command, "mkdir")) { //创建目录
                scanf("%s", temp);
                mkdir(temp, 2);
            }
            else if (!strcmp(command, "create")) {   //创建文件
                scanf("%s", temp);
                if (strlen(temp)>8)
                {
                    printf("The name of the file is too long");
                }
                create(temp, 1); //1表示创建普通文件
            }
            else if (!strcmp(command, "rmdir")) { //删除空目录
                scanf("%s", temp);
                rmdir(temp);
            }
            else if (!strcmp(command, "rm")) {    //删除文件或目录，不提示
                scanf("%s", temp);
                del(temp);
            }
            else if (!strcmp(command, "open")) {   //打开一个文件
                scanf("%s", temp);
                open_file(temp);
            }
            else if (!strcmp(command, "close")) {   //关闭一个文件
                scanf("%s", temp);
                close_file(temp);
            }
            else if (!strcmp(command, "read")) {   //读一个文件
                scanf("%s", temp);
                read_file(temp);
            }
            else if (!strcmp(command, "write")) {  //写一个文件
                scanf("%s", temp);
                while(getchar() != '\n');
                write_ret = write_file(temp);
            }
            else if (!strcmp(command, "ls")) {   //显示当前目录
                ls();
            }
            else if (!strcmp(command, "format")) {  //格式化硬盘
                printf("\"Format\" means erase all the data in the Disk\n");
                printf("Are you sure? (yes/no):\n");
                fflush(stdin);
                scanf("%s", temp);
                if (!strcmp(temp, "yes")) {
                    format();
                    printf("Format successfully!\n");
                }
                else {
                    printf("Format was canceled\n");
                }
            }
            else if (!strcmp(command, "ckdisk")) {  //检查硬盘
                check_disk();
            }
            else if (!strcmp(command, "help") || !strcmp(command, "h")) { //查看帮助
                help();
            }
            else if (!strcmp(command, "quit") || !strcmp(command, "exit")) { //退出系统
                printf("System exit successfully!\n");
                break;
            }
            else if (!strcmp(command, "chmod")) { //修改权限
                scanf("%s", temp);
                unsigned short mode;
                scanf("%hd", &mode);
                chmod(temp, mode);
            }
            else if (!strcmp(command, "mkusr")) { //创建用户
                mkusr();
            }
            else if (!strcmp(command, "rmusr")) { //删除用户
                rmusr();
            }
            else if (!strcmp(command, "chusrname")) { //修改用户名
                chusrname();
            }
            else if (!strcmp(command, "chusrpwd")) { //修改用户密码
                chusrpwd();
            }
            else if (!strcmp(command, "chusr")){  //切换用户
                chusr_flag = 1;
                break;
            }
            else {
                printf("No this Command, use \"help\" to see the command list!\n");
                //help();
            }
            if (strcmp(command, "write") || write_ret != 0) {    
                while(getchar() != '\n');
            }
        }
        if (chusr_flag)
            continue;
        else
            break;
    }
    return 0;
}
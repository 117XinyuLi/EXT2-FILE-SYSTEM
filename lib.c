#include "lib.h"

// 写组描述符
static void update_group_desc() {
    fp = fopen(current_disk, "rb+");
    fseek(fp, GDT_START, SEEK_SET);
    fwrite(&group_desc_buf, GD_SIZE, 1, fp);
    fflush(fp);
}

// 读组描述符
static void reload_group_desc() {
    fseek(fp, GDT_START, SEEK_SET);
    fread(&group_desc_buf, GD_SIZE, 1, fp);
}

// 写第 i 个 inode
static void update_inode_entry(unsigned short i) {
    fp = fopen(current_disk, "rb+");
    fseek(fp, INODE_TABLE + (i - 1) * INODE_SIZE, SEEK_SET);
    fwrite(&inode_buf, INODE_SIZE, 1, fp);
    fflush(fp);
}

// 读第 i 个 inode
static void reload_inode_entry(unsigned short i) {
    fseek(fp, INODE_TABLE + (i - 1) * INODE_SIZE, SEEK_SET);
    fread(&inode_buf, INODE_SIZE, 1, fp);
}

// 写第 i 个数据块，目录项
static void update_dir(unsigned short i) {
    fp = fopen(current_disk, "rb+");
    fseek(fp, DATA_BLOCK + i * BLOCK_SIZE, SEEK_SET);
    fwrite(dir, BLOCK_SIZE, 1, fp);
    fflush(fp);
}

// 读第 i 个数据块，目录项
static void reload_dir(unsigned short i) {
    fseek(fp, DATA_BLOCK + i * BLOCK_SIZE, SEEK_SET);
    fread(dir, BLOCK_SIZE, 1, fp);
}

// 写 block 位图
static void update_block_bitmap() {
    fp = fopen(current_disk, "rb+");
    fseek(fp, BLOCK_BITMAP, SEEK_SET);
    fwrite(bitbuf, BLOCK_SIZE, 1, fp);
    fflush(fp);
}

// 读 block 位图
static void reload_block_bitmap() {
    fseek(fp, BLOCK_BITMAP, SEEK_SET);
    fread(bitbuf, BLOCK_SIZE, 1, fp);
}

// 写 inode 位图
static void update_inode_bitmap() {
    fp = fopen(current_disk, "rb+");
    fseek(fp, INODE_BITMAP, SEEK_SET);
    fwrite(ibuf, BLOCK_SIZE, 1, fp);
    fflush(fp);
}

// 读 inode 位图
static void reload_inode_bitmap() {
    fseek(fp, INODE_BITMAP, SEEK_SET);
    fread(ibuf, BLOCK_SIZE, 1, fp);
}

// 写第 i 个数据块, 从 Buffer 写入
static void update_block(unsigned short i) {
    fp = fopen(current_disk, "rb+");
    fseek(fp, DATA_BLOCK + i * BLOCK_SIZE, SEEK_SET);
    //fseek(fp,0,SEEK_SET);
    fwrite(Buffer, BLOCK_SIZE, 1, fp);
    fflush(fp);
}

// 读第 i 个数据块, 读到 Buffer
static void reload_block(unsigned short i) {
    fseek(fp, DATA_BLOCK + i * BLOCK_SIZE, SEEK_SET);
    fread(Buffer, BLOCK_SIZE, 1, fp);
}

// 写第 i 个数据块, 从 inode_ptr_buffer 写入
static void update_block_from_inode_ptr(unsigned short i) {
    fp = fopen(current_disk, "rb+");
    fseek(fp, DATA_BLOCK + i * BLOCK_SIZE, SEEK_SET);
    fwrite(inode_ptr_buf, BLOCK_SIZE, 1, fp);
    fflush(fp);
}

// 读第 i 个数据块, 读到 inode_ptr_buffer
static void reload_block_to_inode_ptr(unsigned short i) {
    fseek(fp, DATA_BLOCK + i * BLOCK_SIZE, SEEK_SET);
    fread(inode_ptr_buf, BLOCK_SIZE, 1, fp);
}

// 写第 i 个数据块, 从 inode_ptr_buffer_level2 写入
static void update_block_from_inode_ptr_level_2(unsigned short i) {
    fp = fopen(current_disk, "rb+");
    fseek(fp, DATA_BLOCK + i * BLOCK_SIZE, SEEK_SET);
    fwrite(inode_ptr_buf_level_2, BLOCK_SIZE, 1, fp);
    fflush(fp);
}

// 读第 i 个数据块, 读到 inode_ptr_buffer_level2
static void reload_block_to_inode_ptr_level_2(unsigned short i) {
    fseek(fp, DATA_BLOCK + i * BLOCK_SIZE, SEEK_SET);
    fread(inode_ptr_buf_level_2, BLOCK_SIZE, 1, fp);
}

// 分配一个数据块,返回数据块号
static int alloc_block() {
    //bitbuf共有512个字节，表示4096个数据块。根据last_alloc_block/8计算它在bitbuf的哪一个字节
    unsigned short cur = last_alloc_block;
    unsigned char con = 128; // 1000 0000b
    int flag = 0;
    if (group_desc_buf.bg_free_blocks_count == 0) {
        printf("There is no block to be allocated!\n");
        return (0);
    }
    reload_block_bitmap();
    cur /= 8;
    while (bitbuf[cur] == 255) { //该字节的8个bit都已有数据
        if (cur == 511)
            cur = 0; //最后一个字节也已经满，从头开始寻找
        else
            cur++;
    }
    while (bitbuf[cur] & con) { //在一个字节中找具体的某一个bit
        con = con / 2;
        flag++;
    }
    bitbuf[cur] = bitbuf[cur] + con;
    last_alloc_block = cur * 8 + flag;

    update_block_bitmap();
    group_desc_buf.bg_free_blocks_count--;
    update_group_desc();
    return last_alloc_block;
}

// 分配一个 inode
static int get_inode() {
    unsigned short cur = last_alloc_inode;
    unsigned char con = 128;
    int flag = 0;
    if (group_desc_buf.bg_free_inodes_count == 0) {
        printf("There is no Inode to be allocated!\n");
        return 0;
    }
    reload_inode_bitmap();

    cur = (cur - 1) / 8;   //第一个标号是1，但是存储是从0开始的
    while (ibuf[cur] == 255) { //先看该字节的8个位是否已经填满
        if (cur == 511)
            cur = 0;
        else
            cur++;
    }
    while (ibuf[cur] & con) { //再看某个字节的具体哪一位没有被占用
        con = con / 2;
        flag++;
    }
    ibuf[cur] = ibuf[cur] + con;
    last_alloc_inode = cur * 8 + flag + 1;
    update_inode_bitmap();
    group_desc_buf.bg_free_inodes_count--;
    update_group_desc();
    return last_alloc_inode;
}

//查找当前目录中名为tmp的文件或目录，并得到该文件的inode号，它在上级目录中的数据块号以及数据块中目录的项号
static unsigned short research_file(char tmp[100], int file_type, unsigned short *inode_num,
                                   unsigned short *block_num, unsigned short *dir_num) {
    unsigned short j, k;
    reload_inode_entry(current_dir); //进入当前目录
    j = 0;
    while (j < inode_buf.i_blocks) {
        reload_dir(inode_buf.i_block[j]);
        k = 0;
        while (k < 32) { //每个数据块最多有32个目录项
            if (!dir[k].inode || dir[k].file_type != file_type || strcmp(dir[k].name, tmp)) {
                k++;
            }
            else {
                *inode_num = dir[k].inode;
                *block_num = j;
                *dir_num = k;
                return 1;
            }
        }
        j++;
    }
    return 0;
}

// 为新增目录或文件分配 dir_entry
static void dir_prepare(unsigned short tmp, unsigned short len, int type, int mode) {
    reload_inode_entry(tmp);

    if (type == 2) { // 目录
        inode_buf.i_size = 32;
        inode_buf.i_blocks = 1;
        inode_buf.i_block[0] = alloc_block();
        time_t now;
        time(&now);
        inode_buf.i_ctime = now;
        inode_buf.i_mtime = now;
        inode_buf.i_atime = now;
        if (inode_buf.i_block[0] == 0) {
            printf("There is no block to be allocated!\n");
            return;
        }
        dir[0].inode = tmp;
        dir[1].inode = current_dir;
        dir[0].name_len = len;
        dir[1].name_len = current_dirlen;
        dir[0].file_type = dir[1].file_type = 2;

        for (type = 2; type < 32; type++)
            dir[type].inode = 0;
        strcpy(dir[0].name, ".");
        strcpy(dir[1].name, "..");
        update_dir(inode_buf.i_block[0]);

        inode_buf.i_mode = 518; // 0000 0010 0000 0110b
    }
    else {
        inode_buf.i_size = 0;
        inode_buf.i_blocks = 0;

        switch (mode)
        {
        case 1:
            inode_buf.i_mode = 257; // 0000 0100 0000 0001b
            break;
        case 2:
            inode_buf.i_mode = 258; // 0000 0100 0000 0010b
            break;
        case 3:
            inode_buf.i_mode = 259; // 0000 0100 0000 0011b
            break;
        case 4:
            inode_buf.i_mode = 260; // 0000 0100 0000 0100b
            break;
        case 5:
            inode_buf.i_mode = 261; // 0000 0100 0000 0101b
            break;
        case 6:
            inode_buf.i_mode = 262; // 0000 0100 0000 0110b
            break;
        case 7:
            inode_buf.i_mode = 263; // 0000 0100 0000 0111b
            break;
        default:
            break;
        }


        
        time_t now;
        time(&now);
        inode_buf.i_ctime = now;
        inode_buf.i_mtime = now;
        inode_buf.i_atime = now;
        
    }
    update_inode_entry(tmp);
}

// 删除一个块
static void remove_block(unsigned short del_num) {
    unsigned short tmp;
    tmp = del_num / 8;
    reload_block_bitmap();
    switch (del_num % 8) { // 更新block位图 将具体的位置为0
        case 0:
            bitbuf[tmp] = bitbuf[tmp] & 127;
            break; // bitbuf[tmp] & 0111 1111b
        case 1:
            bitbuf[tmp] = bitbuf[tmp] & 191;
            break; //bitbuf[tmp]  & 1011 1111b
        case 2:
            bitbuf[tmp] = bitbuf[tmp] & 223;
            break; //bitbuf[tmp]  & 1101 1111b
        case 3:
            bitbuf[tmp] = bitbuf[tmp] & 239;
            break; //bitbuf[tmp]  & 1110 1111b
        case 4:
            bitbuf[tmp] = bitbuf[tmp] & 247;
            break; //bitbuf[tmp]  & 1111 0111b
        case 5:
            bitbuf[tmp] = bitbuf[tmp] & 251;
            break; //bitbuf[tmp]  & 1111 1011b
        case 6:
            bitbuf[tmp] = bitbuf[tmp] & 253;
            break; //bitbuf[tmp]  & 1111 1101b
        case 7:
            bitbuf[tmp] = bitbuf[tmp] & 254;
            break; // bitbuf[tmp] & 1111 1110b
    }
    update_block_bitmap();
    group_desc_buf.bg_free_blocks_count++;
    update_group_desc();
}

// 删除一个 inode
static void remove_inode(unsigned short del_num) {
    unsigned short tmp;
    tmp = (del_num - 1) / 8;
    reload_inode_bitmap();
    switch ((del_num - 1) % 8) { //更改block位图
        case 0:
            bitbuf[tmp] = bitbuf[tmp] & 127;
            break;
        case 1:
            bitbuf[tmp] = bitbuf[tmp] & 191;
            break;
        case 2:
            bitbuf[tmp] = bitbuf[tmp] & 223;
            break;
        case 3:
            bitbuf[tmp] = bitbuf[tmp] & 239;
            break;
        case 4:
            bitbuf[tmp] = bitbuf[tmp] & 247;
            break;
        case 5:
            bitbuf[tmp] = bitbuf[tmp] & 251;
            break;
        case 6:
            bitbuf[tmp] = bitbuf[tmp] & 253;
            break;
        case 7:
            bitbuf[tmp] = bitbuf[tmp] & 254;
            break;
    }
    update_inode_bitmap();
    group_desc_buf.bg_free_inodes_count++;
    update_group_desc();
}

// 在打开文件表中查找是否已打开文件
static unsigned short search_file(unsigned short Inode) {
    unsigned short fopen_table_point = 0;
    while (fopen_table_point < 16 && fopen_table[fopen_table_point++] != Inode);
    if (fopen_table_point == 16) {
        return 0;
    }
    return 1;
}

// 初始化磁盘
void initialize_disk() {
    int i = 0;
    printf("Creating the ext2 file system\n");
    printf("Please wait ...\n");
    last_alloc_inode = 1;
    last_alloc_block = 0;
    for (i = 0; i < FOPEN_TABLE_MAX; i++) {
        fopen_table[i] = 0; //清空缓冲表
    }
    for (i = 0; i < BLOCK_SIZE; i++) {
        Buffer[i] = 0; // 清空缓冲区
    }
    if (fp != NULL) {
        fclose(fp);
    }
    //printf("current disk: %s\n", current_disk);
    fp = fopen(current_disk, "w+"); //此文件大小是4096*512=B, 用此文件来模拟文件系统
    fseek(fp, DISK_START, SEEK_SET);//将文件指针从0开始
    for (i = 0; i < NUM_BLOCK ; i++) {
        //清空文件, 即清空磁盘全部用0填充, Buffer为缓冲区起始地址, BLOCK_SIZE表示读取大小, 1表示写入对象的个数
        fwrite(Buffer, BLOCK_SIZE, 1, fp);
    }

    // 根目录的inode号为1
    reload_inode_entry(1);

    reload_dir(0);
    // 修改路径名为根目录
    strcpy(current_path, "");
    strcat(current_path, "[");
    strcat(current_path, current_user);
    strcat(current_path, "@ext2 /");

    // 初始化组描述符内容
    reload_group_desc();
    group_desc_buf.bg_block_bitmap = BLOCK_BITMAP; //第一个块位图的起始地址
    group_desc_buf.bg_inode_bitmap = INODE_BITMAP; //第一个inode位图的起始地址
    group_desc_buf.bg_inode_table = INODE_TABLE;   //inode表的起始地址
    group_desc_buf.bg_free_blocks_count = DATA_BLOCK_COUNTS; //空闲数据块数
    group_desc_buf.bg_free_inodes_count = INODE_TABLE_COUNTS; //空闲inode数
    group_desc_buf.bg_used_dirs_count = 0; // 初始分配给目录的节点数是0
    update_group_desc(); // 更新组描述符内容

    reload_block_bitmap();
    reload_inode_bitmap();

    inode_buf.i_mode = 518; // 0000 0010 0000 0110b 因为是目录文件类型, 所以第10位为1
    inode_buf.i_blocks = 0;
    inode_buf.i_size = 32; // 32B因为有两个目录项 . 和 ..
    inode_buf.i_atime = 0;
    inode_buf.i_ctime = 0;
    inode_buf.i_mtime = 0;
    inode_buf.i_dtime = 0;
    inode_buf.i_block[0] = alloc_block(); //分配数据块
    inode_buf.i_blocks++;
    current_dir = get_inode(); //当前目录的inode号
    update_inode_entry(current_dir);

    //初始化根目录的目录项
    dir[0].inode = dir[1].inode = current_dir; //当前目录的inode号
    dir[0].name_len = 0; // .的长度为0
    dir[1].name_len = 0; // ..的长度为0
    dir[0].file_type = dir[1].file_type = 2; // 2表示目录文件
    strcpy(dir[0].name, ".");
    strcpy(dir[1].name, "..");
    update_dir(inode_buf.i_block[0]);
    printf("The ext2 file system has been installed!\n");
    // check_disk();
    // fclose(fp);
}

// 初始化用户信息
void initialize_user() {
    // 从userlist.txt中读取用户信息
    FILE *fp = fopen("./userlist.txt", "r");
    if (fp == NULL) {
        printf("No user in userlist.txt! create an admin!\n");
        strcpy(User[0].username, "admin");
        strcpy(User[0].password, "admin");
        strcpy(User[0].disk_name, "Disk0");

        fp = fopen("./userlist.txt", "w");
        if (fp == NULL) {
            printf("Can't open userlist.txt!\n");
            exit(0);
        }
        fprintf(fp, "%s %s %s\n", User[0].username, User[0].password, User[0].disk_name);
        fclose(fp);
        user_num = 1;
        return;
    }
    int i = 0;
    while (!feof(fp) && i < USER_MAX) {
        fscanf(fp, "%s %s %s\n", User[i].username, User[i].password, User[i].disk_name);
        // printf("%s\n %s\n %s\n", User[i].username, User[i].password, User[i].disk_name);
        i++;
    }
    fclose(fp);

    user_num = i;
    // printf("user_num = %d\n", user_num);
}

// 用户登录
int login(char username[10], char password[10]) {
    for (int i = 0; i < user_num; i++) {
        if (!strcmp(User[i].username, username)) {
            if (!strcmp(User[i].password, password)){
                user_index = i;
                // printf("user_index = %d\n", user_index);
                return 1;
            }
            break;
        }
    }
    return 0;
}

// 初始化内存
void initialize_memory() {
    int i = 0;
    last_alloc_inode = 1;
    last_alloc_block = 0;
    for (i = 0; i < FOPEN_TABLE_MAX; i++) {
        fopen_table[i] = 0;
    }
    strcpy(current_path, "");
    strcat(current_path, "[");
    strcat(current_path, current_user);
    strcat(current_path, "@ext2 /");
    current_dir = 1;
    fp = fopen(current_disk, "rb+");
    if (fp == NULL) {
        printf("The File system does not exist!\n");
        initialize_disk();
        return;
    }

    // 如果文件全部为空，需要重新初始化
    fseek(fp, 0, 0);
    char c;
    int flag = 0;
    while (!feof(fp)) {
        fread(&c, 1, 1, fp);
        if (c != 0) {
            flag = 1;
            break;
        }
    }
    if (flag == 0) {
        printf("The File system does not exist!\n");
        initialize_disk();
        return;
    }

    reload_group_desc();
}

// 格式化
void format() {
    initialize_disk();
    initialize_memory();
}

// 进入某个目录,单步操作
static int cd_onestep(char tmp[100]) {
    unsigned short i, j, k, flag;
    flag = research_file(tmp, 2, &i, &j, &k); // i是inode号，j是它在本目录的数据块中的偏移量，k是它在本目录的目录项中的偏移量
    // 对于..的情况, i指向下一级的inode号，在dirprepare中设定..的inode号为上级目录的inode号
    // research_file中已加载了inode_entry和dir

    if (flag) {
        current_dir = i;
        if (!strcmp(tmp, "..") && dir[k - 1].name_len) // 到上一级目录, k-1对应 . 的目录项，保证到根目录不再往前
        {
            current_path[strlen(current_path) -  dir[k - 1].name_len - 1] = '\0';
            current_dirlen = dir[k].name_len;
            // 修改访问时间
            reload_inode_entry(current_dir);
            time_t t;
            time(&t);
            inode_buf.i_atime = t;
            update_inode_entry(current_dir);
            return 1;
        }
        else if (!strcmp(tmp, ".")) {
            // 修改访问时间
            reload_inode_entry(current_dir);
            time_t t;
            time(&t);
            inode_buf.i_atime = t;
            update_inode_entry(current_dir);
            return 1;
        }
        else if (strcmp(tmp, "..")) // cd 到子目录
        {
            current_dirlen = strlen(tmp);
            strcat(current_path, tmp);
            strcat(current_path, "/");
            // 修改访问时间
            reload_inode_entry(current_dir);
            time_t t;
            time(&t);
            inode_buf.i_atime = t;
            update_inode_entry(current_dir);
            return 1;
        }
    }
    else {
        printf("The directory %s not exists!\n", tmp);
        return 0;
    }
}

// 进入某个目录
void cd(char tmp[100]) {
    // 以/为分隔符，逐级进入目录
    char *p;
    p = strtok(tmp, "/");
    while (p) {
        int ret  = cd_onestep(p);
        if (ret == 0) {
            return;
        }
        p = strtok(NULL, "/");
    }
}

// 创建目录
void mkdir(char tmp[100], int type) {
    unsigned short tmpno, i, j, k, flag;

    // 当前目录下新增目录或文件
    reload_inode_entry(current_dir);
    if (!research_file(tmp, type, &i, &j, &k)) { // 未找到同名文件
        if (inode_buf.i_size == 4096) { // 目录项已满
            printf("Directory has no room to be alloced!\n");
            return;
        }
        flag = 1;
        if (inode_buf.i_size != inode_buf.i_blocks * 512) { // 目录中有某些块中32个 dir_entry 未满
            i = 0;
            while (flag && i < inode_buf.i_blocks) {
                reload_dir(inode_buf.i_block[i]);
                j = 0;
                while (j < 32) {
                    if (dir[j].inode == 0) {
                        flag = 0; //找到某个未装满目录项的块
                        break;
                    }
                    j++;
                }
                i++;
            }
            tmpno = dir[j].inode = get_inode();

            dir[j].name_len = strlen(tmp);
            dir[j].file_type = type;
            strcpy(dir[j].name, tmp);
            update_dir(inode_buf.i_block[i - 1]);
        }
        else { // 全满 新增加块
            inode_buf.i_block[inode_buf.i_blocks] = alloc_block();
            if (inode_buf.i_block[inode_buf.i_blocks] == 0) {
                printf("No block can be alloced!\n");
                return;
            }
            inode_buf.i_blocks++;
            reload_dir(inode_buf.i_block[inode_buf.i_blocks - 1]);
            tmpno = dir[0].inode = get_inode();
            dir[0].name_len = strlen(tmp);
            dir[0].file_type = type;
            strcpy(dir[0].name, tmp);
            // 初始化新块的其余目录项
            for (flag = 1; flag < 32; flag++) {
                dir[flag].inode = 0;
            }
            update_dir(inode_buf.i_block[inode_buf.i_blocks - 1]);
        }
        inode_buf.i_size += 16;

        update_inode_entry(current_dir);

        // 为新增目录分配 dir_entry
        dir_prepare(tmpno, strlen(tmp), type, 6);
    }
    else { // 已经存在同名文件或目录
        printf("Directory has already existed!\n");
    }
}

// 创建文件
void create(char tmp[100], int type) {
    unsigned short tmpno, i, j, k, flag;
    reload_inode_entry(current_dir);
    if (!research_file(tmp, type, &i, &j, &k)) {
        if (inode_buf.i_size == 4096) { // 目录项已满
            printf("Directory has no room to be alloced!\n");
            return;
        }
        flag = 1;
        if (inode_buf.i_size != inode_buf.i_blocks * 512) { // 目录中有某些块中32个 dir_entry 未满
            i = 0;
            while (flag && i < inode_buf.i_blocks) {
                reload_dir(inode_buf.i_block[i]);
                j = 0;
                while (j < 32) { //找到某个未装满目录项的块
                    if (dir[j].inode == 0) {//找到了未分配的目录项
                        flag = 0;
                        break;
                    }
                    j++;
                }
                i++;
            }
            tmpno = dir[j].inode = get_inode();//分配一个新的inode项
            dir[j].name_len = strlen(tmp);
            dir[j].file_type = type;
            strcpy(dir[j].name, tmp);
            update_dir(inode_buf.i_block[i - 1]); //-1是因为i++了
        }
        else { //分配一个新的数据块
            inode_buf.i_block[inode_buf.i_blocks] = alloc_block();
            if (inode_buf.i_block[inode_buf.i_blocks] == 0) {
                printf("No block can be alloced!\n");
                return;
            }
            inode_buf.i_blocks++;
            reload_dir(inode_buf.i_block[inode_buf.i_blocks - 1]);
            tmpno = dir[0].inode = get_inode();
            dir[0].name_len = strlen(tmp);
            dir[0].file_type = type;
            strcpy(dir[0].name, tmp);
            //初始化新块其他项目为0，即未分配，因为写入的时候按块写入，所以要初始化
            for (flag = 1; flag < 32; flag++) {
                dir[flag].inode = 0;
            }
            update_dir(inode_buf.i_block[inode_buf.i_blocks - 1]);
        }
        inode_buf.i_size += 16; //目录项大小为16
        update_inode_entry(current_dir);

        int mode = 0;
        // 如果是.exe/.bin/.com或者不带后缀的文件，就赋予可执行权限
        int len = strlen(tmp);
        if (tmp[len - 1] == 'e' && tmp[len - 2] == 'x' && tmp[len - 3] == 'e' && tmp[len - 4] == '.') {
            mode = 7;
        }
        else if (tmp[len - 1] == 'n' && tmp[len - 2] == 'i' && tmp[len - 3] == 'b' && tmp[len - 4] == '.') {
            mode = 7;
        }
        else if (tmp[len - 1] == 'm' && tmp[len - 2] == 'o' && tmp[len - 3] == 'c' && tmp[len - 4] == '.') {
            mode = 7;
        }
        else {
            mode = 7;
            for (int i = 0; i < len; i++) {
                if (tmp[i] == '.') {
                    mode = 6;
                    break;
                }
            }
        }
        
        //将新增文件的inode节点初始化
        dir_prepare(tmpno, strlen(tmp), type, mode);
    }
    else {
        printf("File has already existed!\n");
    }
}

// 递归删除目录
static void remove_dir(unsigned short inode_no) {
    unsigned short i, j, k;
    reload_inode_entry(inode_no);
    current_dir = inode_no;
    for (i = 0; i < inode_buf.i_blocks; i++) {
        reload_dir(inode_buf.i_block[i]);
        for (j = 0; j < 32; j++) {
            reload_inode_entry(inode_no);
            current_dir = inode_no;
            if (dir[j].inode != 0 && strcmp(dir[j].name, ".") && strcmp(dir[j].name, "..")) {
                if (dir[j].file_type == 1) { // 文件
                    //printf("remove file %s\n", dir[j].name);
                    del(dir[j].name);
                }
                else if (dir[j].file_type == 2) { // 目录
                    //printf("remove dir %s\n", dir[j].name);
                    reload_inode_entry(dir[j].inode);
                    current_dir = dir[j].inode;
                    if (inode_buf.i_size == 32) { // 只有.and ..
                        inode_buf.i_size = 0;
                        inode_buf.i_blocks = 0;
                        remove_block(inode_buf.i_block[0]);
                    }
                    else {
                        remove_dir(dir[j].inode);
                    }
                }
            }
        }
        remove_block(inode_buf.i_block[i]);
    }
    remove_inode(inode_no);
}

// 删除目录
void rmdir(char tmp[100]) {
    unsigned short i, j, k, flag;
    unsigned short m, n;
    if (!strcmp(tmp, "..") || !strcmp(tmp, ".")) {
        printf("The directory can not be deleted!\n");
        return;
    }
    flag = research_file(tmp, 2, &i, &j, &k);
    if (flag) {
        reload_inode_entry(dir[k].inode); // 加载要删除的节点
        if (inode_buf.i_size == 32) { // 只有.and ..
            inode_buf.i_size = 0;
            inode_buf.i_blocks = 0;

            remove_block(inode_buf.i_block[0]);
            // 更新 tmp 所在父目录
            reload_inode_entry(current_dir);
            reload_dir(inode_buf.i_block[j]);
            remove_inode(dir[k].inode);
            dir[k].inode = 0;
            dir[k].name_len = 0;
            dir[k].file_type = 0;
            dir[k].name[0] = '\0';
            dir[k].rec_len = 0;
            update_dir(inode_buf.i_block[j]);
            inode_buf.i_size -= 16;
    
            m = 1;
            while (m < inode_buf.i_blocks) {
                flag = n = 0;
                reload_dir(inode_buf.i_block[m]);
                while (n < 32) {
                    if (!dir[n].inode) {
                        flag++;
                    }
                    n++;
                }
                //如果删除过后，整个数据块的目录项全都为空。类似于在数组中删除某一个位置
                if (flag == 32) {
                    remove_block(inode_buf.i_block[m]);
                    inode_buf.i_blocks--;
                    while (m < inode_buf.i_blocks) {
                        inode_buf.i_block[m] = inode_buf.i_block[m + 1];
                        ++m;
                    }
                }
            }
            update_inode_entry(current_dir);
            return;
        }
        else {
            // 保存当前目录
            unsigned short tmp_dir = current_dir;
            char tmp_name[100];
            strcpy(tmp_name, current_path);
            
            // 如果目录不为空, 递归删除
            remove_dir(dir[k].inode);

            // 恢复当前目录
            current_dir = tmp_dir;
            strcpy(current_path, tmp_name);
            reload_inode_entry(current_dir);
            reload_dir(inode_buf.i_block[j]);
            dir[k].inode = 0;
            dir[k].name_len = 0;
            dir[k].file_type = 0;
            dir[k].name[0] = '\0';
            dir[k].rec_len = 0;
            update_dir(inode_buf.i_block[j]);
            inode_buf.i_size -= 16;

            m = 1;
            while (m < inode_buf.i_blocks) {
                flag = n = 0;
                reload_dir(inode_buf.i_block[m]);
                while (n < 32) {
                    if (!dir[n].inode) {
                        flag++;
                    }
                    n++;
                }
                //如果删除过后，整个数据块的目录项全都为空。类似于在数组中删除某一个位置
                if (flag == 32) {
                    remove_block(inode_buf.i_block[m]);
                    inode_buf.i_blocks--;
                    while (m < inode_buf.i_blocks) {
                        inode_buf.i_block[m] = inode_buf.i_block[m + 1];
                        ++m;
                    }
                }
            }
            update_inode_entry(current_dir);
            return;
            
        }
    }
    else {
        printf("Directory to be deleted not exists!\n");
    }
}

// 删除文件
void del(char tmp[100]) {
    unsigned short i, j, k, m, n, flag;
    m = 0;
    flag = research_file(tmp, 1, &i, &j, &k);
    if (flag) {
        flag = 0;
        // 若文件 tmp 已打开, 则将对应的 fopen_table 项清0
        while (fopen_table[flag] != dir[k].inode && flag < 16) {
            flag++;
        }
        if (flag < 16) {
            fopen_table[flag] = 0;
        }
        reload_inode_entry(i); // 加载删除文件 inode

        // 将现有的数据块全部释放
        if (inode_buf.i_blocks <=6){
            while (inode_buf.i_blocks > 0) {
                remove_block(inode_buf.i_block[inode_buf.i_blocks - 1]);
                inode_buf.i_blocks--;
            }
        }
        else if (inode_buf.i_blocks <= 6 + 256){
            // 删除间接索引块
            reload_block_to_inode_ptr(inode_buf.i_block[6]);
            remove_block(inode_buf.i_block[6]);
            for (int i = 0; i < 64; i++) {
                for (int j = 0; j < 8; j++) {
                    if (inode_ptr_buf[i].i_block[j] != 0) {
                        remove_block(inode_ptr_buf[i].i_block[j]);
                        inode_buf.i_blocks--;
                    }
                }
            }
            inode_buf.i_block[6] = 0;
            while (inode_buf.i_blocks > 0) {
                remove_block(inode_buf.i_block[inode_buf.i_blocks - 1]);
                inode_buf.i_blocks--;
            }

        }
        else if (inode_buf.i_blocks <= 6 + 256 + 256 * 256){
            // 删除间接索引块
            reload_block_to_inode_ptr(inode_buf.i_block[6]);
            remove_block(inode_buf.i_block[6]);
            for (int i = 0; i < 64; i++) {
                for (int j = 0; j < 8; j++) {
                    if (inode_ptr_buf[i].i_block[j] != 0) {
                        remove_block(inode_ptr_buf[i].i_block[j]);
                        inode_buf.i_blocks--;
                    }
                }
            }
            inode_buf.i_block[6] = 0;
            // 删除二次间接索引块
            reload_block_to_inode_ptr(inode_buf.i_block[7]);
            for (int i = 0; i < 64; i++) {
                for (int j = 0; j < 8; j++) {
                    if (inode_ptr_buf[i].i_block[j] != 0) {
                        reload_block_to_inode_ptr_level_2(inode_ptr_buf[i].i_block[j]);
                        for (int k = 0; k < 64; k++) {
                            for (int l = 0; l < 8; l++) {
                                if (inode_ptr_buf_level_2[k].i_block[l] != 0) {
                                    remove_block(inode_ptr_buf_level_2[k].i_block[l]);
                                    inode_buf.i_blocks--;
                                }
                            }
                        }
                        remove_block(inode_ptr_buf[i].i_block[j]);
                    }
                }
            }
            remove_block(inode_buf.i_block[7]);
            inode_buf.i_block[7] = 0;
            while (inode_buf.i_blocks > 0) {
                remove_block(inode_buf.i_block[inode_buf.i_blocks - 1]);
                inode_buf.i_blocks--;
            }
        }

        inode_buf.i_blocks = 0;
        inode_buf.i_size = 0;
        remove_inode(i);
        // 更新父目录
        reload_inode_entry(current_dir);
        reload_dir(inode_buf.i_block[j]);
        dir[k].inode = 0; //删除inode节点
        dir[k].file_type = 0;
        dir[k].name[0] = '\0';
        dir[k].name_len = 0;
        dir[k].rec_len = 0;
        update_dir(inode_buf.i_block[j]);
        inode_buf.i_size -= 16;
        m = 1;
        //删除一项后整个数据块为空，则将该数据块删除
        while (m < inode_buf.i_blocks) {
            flag = n = 0;
            reload_dir(inode_buf.i_block[m]);
            while (n < 32) {
                if (!dir[n].inode) {
                    flag++;
                }
                n++;
            }
            if (flag == 32) {
                remove_block(inode_buf.i_block[m]);
                inode_buf.i_blocks--;
                while (m < inode_buf.i_blocks) {
                    inode_buf.i_block[m] = inode_buf.i_block[m + 1];
                    ++m;
                }
            }
        }
        update_inode_entry(current_dir);
    }
    else {
        printf("The file %s not exists!\n", tmp);
    }
}

// 打开文件
void open_file(char tmp[100]) {
    unsigned short flag, i, j, k;
    flag = research_file(tmp, 1, &i, &j, &k);
    if (flag) {
        if (search_file(dir[k].inode)) {
            printf("The file %s has opened!\n", tmp);
        }
        else {
            flag = 0;
            while (fopen_table[flag]) {
                flag++;
            }
            fopen_table[flag] = (short) dir[k].inode;

            // 更新文件的访问时间
            reload_inode_entry(dir[k].inode);
            time_t t;
            time(&t);
            inode_buf.i_atime = t;
            update_inode_entry(dir[k].inode);

            printf("File %s opened!\n", tmp);
        }
    }
    else
        printf("The file %s does not exist!\n", tmp);
}

// 关闭文件
void close_file(char tmp[100]) {
    unsigned short flag, i, j, k;
    flag = research_file(tmp, 1, &i, &j, &k);

    if (flag) {
        if (search_file(dir[k].inode)) {
            flag = 0;
            while (fopen_table[flag] != dir[k].inode) {
                flag++;
            }
            fopen_table[flag] = 0;
            printf("File %s closed!\n", tmp);
        }
        else {
            printf("The file %s has not been opened!\n", tmp);
        }
    }
    else {
        printf("The file %s does not exist!\n", tmp);
    }
}

// 读文件
void read_file(char tmp[100]) {
    unsigned short flag, i, j, k, t;
    flag = research_file(tmp, 1, &i, &j, &k);
    if (flag) {
        if (search_file(dir[k].inode)) //读文件的前提是该文件已经打开
        {
            reload_inode_entry(dir[k].inode);
            // 修改文件的访问时间
            time_t t;
            time(&t);
            inode_buf.i_atime = t;
            update_inode_entry(dir[k].inode);
            //判断是否有读的权限
            if (!(inode_buf.i_mode & 4)) // i_mode:111b:读,写,执行
            {
                printf("The file %s can not be read!\n", tmp);
                return;
            }
            if (inode_buf.i_size == 0) {
                printf("The file %s is empty!\n", tmp);
                return;
            }

            if (inode_buf.i_blocks <= 6){
                for (flag = 0; flag < inode_buf.i_blocks; flag++) {
                    reload_block(inode_buf.i_block[flag]);
                    for (t = 0; t < (inode_buf.i_size - flag * 512 > 512 ? 512 : inode_buf.i_size - flag * 512); t++) {
                        printf("%c", Buffer[t]);
                    }
                }
                printf("\n");
                return;
            }
            else if (inode_buf.i_blocks <= 6+256){
                unsigned long size = inode_buf.i_size;
                for (flag = 0; flag < 6; flag++) {
                    reload_block(inode_buf.i_block[flag]);
                    for (t = 0; t < 512; t++) {
                        printf("%c", Buffer[t]);
                        size--;
                    }
                }
                reload_block_to_inode_ptr(inode_buf.i_block[6]);
                for (int i = 0; i < 64; i++) {
                    for (int j = 0; j < 8; j++) {
                        if (inode_ptr_buf[i].i_block[j] != 0) {
                            reload_block(inode_ptr_buf[i].i_block[j]);
                            for (t = 0; t < 512; t++) {
                                printf("%c", Buffer[t]);
                                size--;
                                if (size == 0){
                                    printf("\n");
                                    return;
                                }
                            }
                        }
                    }
                }
            }
            else if (inode_buf.i_blocks <= 6+256+256*256){
                unsigned long size = inode_buf.i_size;
                for (flag = 0; flag < 6; flag++) {
                    reload_block(inode_buf.i_block[flag]);
                    for (t = 0; t < 512; t++) {
                        printf("%c", Buffer[t]);
                        size--;
                    }
                }
                reload_block_to_inode_ptr(inode_buf.i_block[6]);
                for (int i = 0; i < 64; i++) {
                    for (int j = 0; j < 8; j++) {
                        if (inode_ptr_buf[i].i_block[j] != 0) {
                            reload_block(inode_ptr_buf[i].i_block[j]);
                            for (t = 0; t < 512; t++) {
                                printf("%c", Buffer[t]);
                                size--;
                            }
                        }
                    }
                }
                reload_block_to_inode_ptr(inode_buf.i_block[7]);
                for (int i = 0; i < 64; i++) {
                    for (int j = 0; j < 8; j++) {
                        if (inode_ptr_buf[i].i_block[j] != 0) {
                            reload_block_to_inode_ptr_level_2(inode_ptr_buf[i].i_block[j]);
                            for (int k = 0; k < 64; k++) {
                                for (int l = 0; l < 8; l++) {
                                    if (inode_ptr_buf_level_2[k].i_block[l] != 0) {
                                        reload_block(inode_ptr_buf_level_2[k].i_block[l]);
                                        for (t = 0; t < 512; t++) {
                                            printf("%c", Buffer[t]);
                                            size--;
                                            if (size == 0){
                                                printf("\n");
                                                return;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else{
                printf("The file is too large to read!\n");
            }
        }
        else {
            printf("The file %s has not been opened!\n", tmp);
        }
    }
    else
        printf("The file %s not exists!\n", tmp);
}

// 文件以覆盖方式写入
int write_file(char tmp[100]) {
    unsigned short flag, i, j, k, size = 0, need_blocks, length;
    flag = research_file(tmp, 1, &i, &j, &k);
    if (flag) {
        if (search_file(dir[k].inode)) {
            reload_inode_entry(dir[k].inode);
            // 修改文件的访问时间和修改时间
            time_t t;
            time(&t);
            inode_buf.i_atime = t;
            inode_buf.i_mtime = t;
            update_inode_entry(dir[k].inode);
            //判断是否有写的权限
            if (!(inode_buf.i_mode & 2)) // i_mode:111b:读,写,执行
            {
                printf("The file %s can not be writed!\n", tmp);
                return 0;
            }
            //fflush(stdin);
            while (1) {
                tempbuf[size] = getchar();
                if (tempbuf[size] == '#') {
                    tempbuf[size] = '\0';
                    break;
                }
                if (size >= MAX_FILE_SIZE) {
                    printf("Sorry,the file is too large!\n");
                    break;
                }
                size++;
            }
            length = strlen(tempbuf) + 1;
            //计算需要的数据块数目
            need_blocks = length / 512;
            if (length % 512) {
                need_blocks++;
            }
            if (need_blocks <= 6) // 直接索引，使用6个直接索引块
            {
                // 分配文件所需块数目
                //因为以覆盖写的方式写，要先判断原有的数据块数目
                if (inode_buf.i_blocks <= need_blocks) {
                    while (inode_buf.i_blocks < need_blocks) {
                        inode_buf.i_block[inode_buf.i_blocks] = alloc_block();
                        if(!inode_buf.i_block[inode_buf.i_blocks])
                        {
                            printf("No enough space!\n");
                            return 2;
                        }
                        inode_buf.i_blocks++;
                    }
                }
                else {
                    if (inode_buf.i_blocks <=6){
                            while (inode_buf.i_blocks > need_blocks) {
                            remove_block(inode_buf.i_block[inode_buf.i_blocks - 1]);
                            inode_buf.i_blocks--;
                        }
                    }
                    else if (need_blocks <= 6 + 256){
                        // 删除间接索引块
                        reload_block_to_inode_ptr(inode_buf.i_block[6]);
                        remove_block(inode_buf.i_block[6]);
                        for (int i = 0; i < 64; i++) {
                            for (int j = 0; j < 8; j++) {
                                if (inode_ptr_buf[i].i_block[j] != 0) {
                                    remove_block(inode_ptr_buf[i].i_block[j]);
                                    inode_buf.i_blocks--;
                                }
                            }
                        }
                        inode_buf.i_block[6] = 0;
                        while (inode_buf.i_blocks > need_blocks) {
                            remove_block(inode_buf.i_block[inode_buf.i_blocks - 1]);
                            inode_buf.i_blocks--;
                        }
                    }
                    else if (need_blocks <= 6 + 256 + 256 * 256){
                        // 删除间接索引块
                        reload_block_to_inode_ptr(inode_buf.i_block[6]);
                        remove_block(inode_buf.i_block[6]);
                        for (int i = 0; i < 64; i++) {
                            for (int j = 0; j < 8; j++) {
                                if (inode_ptr_buf[i].i_block[j] != 0) {
                                    remove_block(inode_ptr_buf[i].i_block[j]);
                                    inode_buf.i_blocks--;
                                }
                            }
                        }
                        inode_buf.i_block[6] = 0;
                        // 删除二次间接索引块
                        reload_block_to_inode_ptr(inode_buf.i_block[7]);
                        for (int i = 0; i < 64; i++) {
                            for (int j = 0; j < 8; j++) {
                                if (inode_ptr_buf[i].i_block[j] != 0) {
                                    reload_block_to_inode_ptr_level_2(inode_ptr_buf[i].i_block[j]);
                                    for (int k = 0; k < 64; k++) {
                                        for (int l = 0; l < 8; l++) {
                                            if (inode_ptr_buf_level_2[k].i_block[l] != 0) {
                                                remove_block(inode_ptr_buf_level_2[k].i_block[l]);
                                                inode_buf.i_blocks--;
                                            }
                                        }
                                    }
                                    remove_block(inode_ptr_buf[i].i_block[j]);
                                }
                            }
                        }
                        remove_block(inode_buf.i_block[7]);
                        inode_buf.i_block[7] = 0;
                        while (inode_buf.i_blocks > need_blocks) {
                            remove_block(inode_buf.i_block[inode_buf.i_blocks - 1]);
                            inode_buf.i_blocks--;
                        }
                    }
                }
                j = 0;
                while (j < need_blocks) {
                    if (j != need_blocks - 1) {
                        reload_block(inode_buf.i_block[j]);
                        memcpy(Buffer, tempbuf + j * BLOCK_SIZE, BLOCK_SIZE);
                        update_block(inode_buf.i_block[j]);
                    }
                    else {
                        reload_block(inode_buf.i_block[j]);
                        memcpy(Buffer, tempbuf + j * BLOCK_SIZE, length - j * BLOCK_SIZE);
                        inode_buf.i_size = length;
                        update_block(inode_buf.i_block[j]);
                    }
                    j++;
                }
                update_inode_entry(dir[k].inode);

            }
            else if(need_blocks <= 6 + 256) // 一次间接索引，使用6个直接索引块和1个一次间接索引块
            {
                // 索引块
                short inode_index = alloc_block();
                if(!inode_index)
                {
                    printf("No enough space!\n");
                    return 2;
                }

                // 将现有的数据块全部释放
                inode_buf.i_size = 0;
                if (inode_buf.i_blocks <=6){
                    while (inode_buf.i_blocks > 0) {
                        remove_block(inode_buf.i_block[inode_buf.i_blocks - 1]);
                        inode_buf.i_blocks--;
                    }
                }
                else if (inode_buf.i_blocks <= 6 + 256){
                    // 删除间接索引块
                    reload_block_to_inode_ptr(inode_buf.i_block[6]);
                    remove_block(inode_buf.i_block[6]);
                    for (int i = 0; i < 64; i++) {
                        for (int j = 0; j < 8; j++) {
                            if (inode_ptr_buf[i].i_block[j] != 0) {
                                remove_block(inode_ptr_buf[i].i_block[j]);
                                inode_buf.i_blocks--;
                            }
                        }
                    }
                    inode_buf.i_block[6] = 0;
                    while (inode_buf.i_blocks > 0) {
                        remove_block(inode_buf.i_block[inode_buf.i_blocks - 1]);
                        inode_buf.i_blocks--;
                    }

                }
                else if (inode_buf.i_blocks <= 6 + 256 + 256 * 256){
                    // 删除间接索引块
                    reload_block_to_inode_ptr(inode_buf.i_block[6]);
                    remove_block(inode_buf.i_block[6]);
                    for (int i = 0; i < 64; i++) {
                        for (int j = 0; j < 8; j++) {
                            if (inode_ptr_buf[i].i_block[j] != 0) {
                                remove_block(inode_ptr_buf[i].i_block[j]);
                                inode_buf.i_blocks--;
                            }
                        }
                    }
                    inode_buf.i_block[6] = 0;
                    // 删除二次间接索引块
                    reload_block_to_inode_ptr(inode_buf.i_block[7]);
                    for (int i = 0; i < 64; i++) {
                        for (int j = 0; j < 8; j++) {
                            if (inode_ptr_buf[i].i_block[j] != 0) {
                                reload_block_to_inode_ptr_level_2(inode_ptr_buf[i].i_block[j]);
                                for (int k = 0; k < 64; k++) {
                                    for (int l = 0; l < 8; l++) {
                                        if (inode_ptr_buf_level_2[k].i_block[l] != 0) {
                                            remove_block(inode_ptr_buf_level_2[k].i_block[l]);
                                            inode_buf.i_blocks--;
                                        }
                                    }
                                }
                                remove_block(inode_ptr_buf[i].i_block[j]);
                            }
                        }
                    }
                    remove_block(inode_buf.i_block[7]);
                    inode_buf.i_block[7] = 0;
                    while (inode_buf.i_blocks > 0) {
                        remove_block(inode_buf.i_block[inode_buf.i_blocks - 1]);
                        inode_buf.i_blocks--;
                    }
                }
                
                // 开辟所有的数据块
                int* all_blocks = (int*)malloc(sizeof(int) * need_blocks);
                for (int i = 0; i < need_blocks; i++){
                    all_blocks[i] = alloc_block();
                    if(!all_blocks[i])
                    {
                        printf("No enough space!\n");
                        return 2;
                    }
                }

                // 将数据块写入
                for (int i = 0; i < need_blocks; i++){
                    if (i != need_blocks - 1) {
                        reload_block(all_blocks[i]);
                        memcpy(Buffer, tempbuf + i * BLOCK_SIZE, BLOCK_SIZE);
                        update_block(all_blocks[i]);
                    }
                    else {
                        reload_block(all_blocks[i]);
                        memcpy(Buffer, tempbuf + i * BLOCK_SIZE, length - i * BLOCK_SIZE);
                        inode_buf.i_size = length;
                        update_block(all_blocks[i]);
                    }
                }

                // 将Buffer全部置零写入 inode_index
                memset(Buffer, 0, BLOCK_SIZE);
                update_block(inode_index);

                // 将前6个直接索引写入 inode_index
                for (int i = 0; i < 6; i++){
                    inode_buf.i_block[i] = all_blocks[i];
                }
                inode_buf.i_blocks = need_blocks;
                inode_buf.i_size = length;
                inode_buf.i_block[6] = inode_index;
                inode_buf.i_block[7] = 0;
                update_inode_entry(dir[k].inode);

                // 将all_blocks写入 inode_ptr_buf
                int block_count = 6;
                for(int i = 0; i < 64; i++){
                    for (int j = 0; j < 8; i++){
                        if (block_count < need_blocks){
                            inode_ptr_buf[i].i_block[j] = all_blocks[block_count];
                            block_count++;
                        }
                        else{
                            inode_ptr_buf[i].i_block[j] = 0;
                        }
                    } 
                }
                update_block_from_inode_ptr(inode_index);

            }
            else if(need_blocks <= 6 + 256 + 256 * 256) // 二次间接索引，使用6个直接索引块，1个一次间接索引块和1个二次间接索引块
            {

                // 将现有的数据块全部释放
                inode_buf.i_size = 0;
                if (inode_buf.i_blocks <=6){
                    while (inode_buf.i_blocks > 0) {
                        remove_block(inode_buf.i_block[inode_buf.i_blocks - 1]);
                        inode_buf.i_blocks--;
                    }
                }
                else if (inode_buf.i_blocks <= 6 + 256){
                    // 删除间接索引块
                    reload_block_to_inode_ptr(inode_buf.i_block[6]);
                    remove_block(inode_buf.i_block[6]);
                    for (int i = 0; i < 64; i++) {
                        for (int j = 0; j < 8; j++) {
                            if (inode_ptr_buf[i].i_block[j] != 0) {
                                remove_block(inode_ptr_buf[i].i_block[j]);
                                inode_buf.i_blocks--;
                            }
                        }
                    }
                    inode_buf.i_block[6] = 0;
                    while (inode_buf.i_blocks > 0) {
                        remove_block(inode_buf.i_block[inode_buf.i_blocks - 1]);
                        inode_buf.i_blocks--;
                    }

                }
                else if (inode_buf.i_blocks <= 6 + 256 + 256 * 256){
                    // 删除间接索引块
                    reload_block_to_inode_ptr(inode_buf.i_block[6]);
                    remove_block(inode_buf.i_block[6]);
                    for (int i = 0; i < 64; i++) {
                        for (int j = 0; j < 8; j++) {
                            if (inode_ptr_buf[i].i_block[j] != 0) {
                                remove_block(inode_ptr_buf[i].i_block[j]);
                                inode_buf.i_blocks--;
                            }
                        }
                    }
                    inode_buf.i_block[6] = 0;
                    // 删除二次间接索引块
                    reload_block_to_inode_ptr(inode_buf.i_block[7]);
                    for (int i = 0; i < 64; i++) {
                        for (int j = 0; j < 8; j++) {
                            if (inode_ptr_buf[i].i_block[j] != 0) {
                                reload_block_to_inode_ptr_level_2(inode_ptr_buf[i].i_block[j]);
                                for (int k = 0; k < 64; k++) {
                                    for (int l = 0; l < 8; l++) {
                                        if (inode_ptr_buf_level_2[k].i_block[l] != 0) {
                                            remove_block(inode_ptr_buf_level_2[k].i_block[l]);
                                            inode_buf.i_blocks--;
                                        }
                                    }
                                }
                                remove_block(inode_ptr_buf[i].i_block[j]);
                            }
                        }
                    }
                    remove_block(inode_buf.i_block[7]);
                    inode_buf.i_block[7] = 0;
                    while (inode_buf.i_blocks > 0) {
                        remove_block(inode_buf.i_block[inode_buf.i_blocks - 1]);
                        inode_buf.i_blocks--;
                    }
                }
                
                // 索引块1
                short inode_index_1 = alloc_block();
                if (!inode_index_1)
                {
                    printf("No enough space!\n");
                    return 2;
                }
                
                // 索引块2
                short needed_index_blocks = (need_blocks - 6 - 256) / 256 + 1;
                short inode_index_2 = alloc_block();
                if (!inode_index_2)
                {
                    printf("No enough space!\n");
                    return 2;
                }
                short* inode_index_2_index = (short*)malloc(sizeof(short) * needed_index_blocks);
                for (int i = 0; i < needed_index_blocks; i++){
                    inode_index_2_index[i] = alloc_block();
                    if (!inode_index_2_index[i])
                    {
                        printf("No enough space!\n");
                        return 2;
                    }
                }
                
                // 开辟所有的数据块
                int* all_blocks = (int*)malloc(sizeof(int) * need_blocks);
                for (int i = 0; i < need_blocks; i++){
                    all_blocks[i] = alloc_block();
                    if(!inode_buf.i_block[inode_buf.i_blocks])
                    {
                        printf("No enough space!\n");
                        return 2;
                    }
                }

                // 将数据块写入
                for (int i = 0; i < need_blocks; i++){
                    if (i != need_blocks - 1) {
                        reload_block(all_blocks[i]);
                        memcpy(Buffer, tempbuf + i * BLOCK_SIZE, BLOCK_SIZE);
                        update_block(all_blocks[i]);
                    }
                    else {
                        reload_block(all_blocks[i]);
                        memcpy(Buffer, tempbuf + i * BLOCK_SIZE, length - i * BLOCK_SIZE);
                        inode_buf.i_size = length;
                        update_block(all_blocks[i]);
                    }
                }

                // 将Buffer全部置零写入 inode_index
                memset(Buffer, 0, BLOCK_SIZE);
                update_block(inode_index_1);
                update_block(inode_index_2);
                for (int i = 0; i < needed_index_blocks; i++){
                    update_block(inode_index_2_index[i]);
                }

                // 将前6个直接索引写入 inode_index
                for (int i = 0; i < 6; i++){
                    inode_buf.i_block[i] = all_blocks[i];
                }
                inode_buf.i_blocks = need_blocks;
                inode_buf.i_size = length;
                inode_buf.i_block[6] = inode_index_1;
                inode_buf.i_block[7] = inode_index_2;
                update_inode_entry(dir[k].inode);

                // 将all_blocks写入 inode_ptr_buf
                int block_count = 6;
                for(int i = 0; i < 64; i++){
                    for (int j = 0; j < 8; i++){
                        if (block_count < need_blocks){
                            inode_ptr_buf[i].i_block[j] = all_blocks[block_count];
                            block_count++;
                        }
                        else{
                            inode_ptr_buf[i].i_block[j] = 0;
                        }
                    } 
                }
                update_block_from_inode_ptr(inode_index_1);

                // 将inode_index_2_index写入 inode_ptr_buf
                int index_block_count = 0;
                for(int i = 0; i < 64; i++){
                    for (int j = 0; j < 8; i++){
                        if (index_block_count < needed_index_blocks){
                            inode_ptr_buf[i].i_block[j] = inode_index_2_index[index_block_count];
                            index_block_count++;
                        }
                        else{
                            inode_ptr_buf[i].i_block[j] = 0;
                        }
                    } 
                }
                update_block_from_inode_ptr(inode_index_2);

                // 将all_blocks写入 inode_ptr_buf
                for (int m = 0; m < needed_index_blocks; m++){
                    for(int i = 0; i < 64; i++){
                        for (int j = 0; j < 8; i++){
                            if (block_count < need_blocks){
                                inode_ptr_buf[i].i_block[j] = all_blocks[block_count];
                                block_count++;
                            }
                            else{
                                inode_ptr_buf[i].i_block[j] = 0;
                            }
                        } 
                    }
                    update_block_from_inode_ptr(inode_index_2_index[m]);
                }

            }
            else {
                printf("Sorry,the size of the file is too large!\n");
                return 3;
            }
            return 1;
        }
        else {
            printf("The file %s has not opened!\n", tmp);
            return 0;
        }
    }
    else {
        printf("The file %s does not exist!\n", tmp);
        return 0;
    }
}

// 递归计算文件夹大小
int get_dir_size(short inode_num) {
    int size = 0;
    reload_inode_entry(inode_num);
    int num_of_files = inode_buf.i_size / 16;
    for (int i = 0; i < num_of_files; i++) {
        reload_inode_entry(inode_num);
        reload_dir(inode_buf.i_block[i/32]);
        if (dir[i%32].file_type == 2 && strcmp(dir[i%32].name, ".") && strcmp(dir[i%32].name, "..")) {
            size += get_dir_size(dir[i%32].inode) + 0;
        }
        else if (!strcmp(dir[i].name, ".") || !strcmp(dir[i].name, "..")) {
            size += 0;
        }
        else {
            reload_inode_entry(dir[i%32].inode);
            size += inode_buf.i_size + 0;
        }
    }
    return size;
}

// 查看目录下的内容
void ls() {
    printf("items\t\ttype\tmode\t   size\t\tcreate time\t\taccess time\t\tmodify time\n");
    unsigned short i, j, k, flag;
    i = 0;
    reload_inode_entry(current_dir);
    while (i < inode_buf.i_blocks) {
        k = 0;
        reload_dir(inode_buf.i_block[i]);
        while (k < 32) {
            if (dir[k].inode) {
                printf("%-10s\t", dir[k].name);
                if (dir[k].file_type == 2) {
                    j = 0;
                    reload_inode_entry(dir[k].inode);
                    if (!strcmp(dir[k].name, "..")) {
                        flag = 1;
                    }
                    else if (!strcmp(dir[k].name, ".")) {
                        flag = 0;
                    }
                    else {
                        flag = 2;
                    }
                    printf("<DIR>\t");
                    switch (inode_buf.i_mode & 7) {
                        case 0:
                            printf("_____\t");
                            break;
                        case 1:
                            printf("____x\t");
                            break;
                        case 2:
                            printf("__w__\t");
                            break;
                        case 3:
                            printf("__w_x\t");
                            break;
                        case 4:
                            printf("r____\t");
                            break;
                        case 5:
                            printf("r___x\t");
                            break;
                        case 6:
                            printf("r_w__\t");
                            break;
                        case 7:
                            printf("r_w_x\t");
                            break;
                    }
                    if (flag != 2) {
                        printf("   ----\t\t");
                    }
                    else {
                        // printf("%ld Bytes\t", inode_buf.i_size);
                        // 递归计算文件大小
                        // 保存当前目录
                        unsigned short tmp_current_dir = current_dir;
                        struct dir_entry tmp_dir[32];
                        int tmp_k = k;
                        for (int i = 0; i < 32; i++) {
                            tmp_dir[i] = dir[i];
                        }

                        // 递归计算文件大小

                        long long size = get_dir_size(dir[k].inode);

                        // 恢复当前目录
                        current_dir = tmp_current_dir;
                        for (int i = 0; i < 32; i++) {
                            dir[i] = tmp_dir[i];
                        }
                        k = tmp_k;

                        // 打印文件大小
                        if (size < 1024) {
                            printf("%4lld Bytes\t", size);
                        }
                        else if (size < 1024 * 1024) {
                            printf("%4lld KB\t", size / 1024);
                        }
                        else if (size < 1024 * 1024 * 1024) {
                            printf("%4lld MB\t", size / 1024 / 1024);
                        }
                        else {
                            printf("%4lld GB\t", size / 1024 / 1024 / 1024);
                        }

                    }
                    if(flag != 2) {
                        printf("----\t\t\t----\t\t\t----");
                    }
                    else {
                        struct tm *t;
                        t = localtime(&inode_buf.i_ctime);
                        printf("%d-%d-%d %d:%d:%d\t", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour,
                               t->tm_min, t->tm_sec);
                        t = localtime(&inode_buf.i_atime);
                        printf("%d-%d-%d %d:%d:%d\t", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour,
                               t->tm_min, t->tm_sec);
                        t = localtime(&inode_buf.i_mtime);
                        printf("%d-%d-%d %d:%d:%d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour,
                               t->tm_min, t->tm_sec);
                    }
                }
                else if (dir[k].file_type == 1) {
                    j = 0;
                    reload_inode_entry(dir[k].inode);
                    printf("<FILE>\t");
                    switch (inode_buf.i_mode & 7) {
                        case 0:
                            printf("_____\t");
                            break;
                        case 1:
                            printf("____x\t");
                            break;
                        case 2:
                            printf("__w__\t");
                            break;
                        case 3:
                            printf("__w_x\t");
                            break;
                        case 4:
                            printf("r____\t");
                            break;
                        case 5:
                            printf("r___x\t");
                            break;
                        case 6:
                            printf("r_w__\t");
                            break;
                        case 7:
                            printf("r_w_x\t");
                            break;
                    }
                    if (inode_buf.i_size < 1024){
                        printf("%4ld Bytes\t", inode_buf.i_size);
                    }
                    else if (inode_buf.i_size < 1024 * 1024){
                        printf("%4ld KB\t", inode_buf.i_size / 1024);
                    }
                    else if (inode_buf.i_size < 1024 * 1024 * 1024){
                        printf("%4ld MB\t", inode_buf.i_size / 1024 / 1024);
                    }
                    else{
                        printf("%4ld GB\t", inode_buf.i_size / 1024 / 1024 / 1024);
                    }
                    struct tm *t;
                    t = localtime(&inode_buf.i_ctime);
                    printf("%d-%d-%d %d:%d:%d\t", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour,
                           t->tm_min, t->tm_sec);
                    t = localtime(&inode_buf.i_atime);
                    printf("%d-%d-%d %d:%d:%d\t", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour,
                           t->tm_min, t->tm_sec);
                    t = localtime(&inode_buf.i_mtime);
                    printf("%d-%d-%d %d:%d:%d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour,
                           t->tm_min, t->tm_sec);
                }
                printf("\n");
            }
            k++;
            reload_inode_entry(current_dir);
        }
        i++;
    }
    printf("\ntips: The size of the file directory entry is not calculated in the file size.\n\n");
}

// 修改文件权限
void chmod(char tmp[100], unsigned short mode) {
    unsigned short flag, i, j, k;
    flag = research_file(tmp, 1, &i, &j, &k);
    if (flag) {
        if (mode < 0 || mode > 7) {
            printf("Wrong mode!\n");
            return;
        }
        reload_inode_entry(dir[k].inode);
        inode_buf.i_mode = mode;
        update_inode_entry(dir[k].inode);
    }
    else
        printf("The file %s does not exist!\n", tmp);
}

// 检查磁盘状态
void check_disk() {
    printf("The disk status is as follows:\n");
    printf("The name of the volume is: %s\n", VOLUME_NAME);
    printf("The name of the disk is: %s\n", current_disk);
    printf("The size of the Block is: %d\n", BLOCK_SIZE);
    printf("The number of the Block is: %d\n", NUM_BLOCK);
    float total = BLOCK_SIZE * NUM_BLOCK / 1024.0 / 1024.0;
    printf("The total size of the disk is: %.2fMB\n", total);
}

// 创建用户
void mkusr() {
    // 只有admin用户才能创建用户
    if(!strcmp(current_user, "admin") && user_num < USER_MAX) {
        printf("Please input the name of the user: ");
        scanf("%s", User[user_num].username);
        // 检查用户名是否已存在
        for(int i = 0; i < user_num; i++) {
            if(!strcmp(User[i].username, User[user_num].username)) {
                printf("The username has already existed!\n");
                return;
            }
        }
        printf("Please input the password of the user: ");
        scanf("%s", User[user_num].password);
        printf("Please input the disk name of the user: ");
        scanf("%s", User[user_num].disk_name);
        // 检查磁盘名是否已存在
        for(int i = 0; i < user_num; i++) {
            if(!strcmp(User[i].disk_name, User[user_num].disk_name)) {
                printf("The disk name has already existed!\n");
                return;
            }
        }

        // 追加写入userlist.txt
        FILE* fp = fopen("./userlist.txt", "a");
        if (fp == NULL) {
            printf("Can't open userlist.txt!\n");
            exit(0);
        }
        fprintf(fp, "%s %s %s\n", User[user_num].username, User[user_num].password, User[user_num].disk_name);
        fclose(fp);
        
        user_num++;
        printf("Create user successfully!\n");
    }
    else {
        printf("Only admin can create user!\n");
    }
}

// 删除用户
void rmusr() {
    // 只有admin用户才能删除用户
    if(!strcmp(current_user, "admin")) {
        char username[20];
        printf("Please input the name of the user: ");
        scanf("%s", username);

        // 不允许删除admin用户
        if(!strcmp(username, "admin")) {
            printf("Can't delete admin!\n");
            return;
        }

        for(int i = 0; i < user_num; i++) {
            if(!strcmp(username, User[i].username)) {
                // 将该文件全部置零
                char buf[BLOCK_SIZE] = {0};
                char tmp[100];
                strcpy(tmp, User[i].disk_name);
                
                FILE* fp = fopen(tmp, "w");
                if (fp == NULL) {
                    printf("Can't open %s!\n", tmp);
                    exit(0);
                }
                for(int j = 0; j < NUM_BLOCK; j++) {
                    fwrite(buf, BLOCK_SIZE, 1, fp);
                }
                fclose(fp);

                // 将后面的用户信息前移
                for(int j = i; j < user_num - 1; j++) {
                    strcpy(User[j].username, User[j+1].username);
                    strcpy(User[j].password, User[j+1].password);
                    strcpy(User[j].disk_name, User[j+1].disk_name);
                }
                user_num--;

                // 重新写入userlist.txt
                fp = fp = fopen("./userlist.txt", "w");
                if (fp == NULL) {
                    printf("Can't open userlist.txt!\n");
                    exit(0);
                }
                for(int j = 0; j < user_num; j++) {
                    fprintf(fp, "%s %s %s\n", User[j].username, User[j].password, User[j].disk_name);
                }
                fclose(fp);

                printf("Delete user successfully!\n");
                return;
            }
        }
        printf("The user %s does not exist!\n", username);
    }
    else {
        printf("Only admin can delete user!\n");
    }
}

// 修改用户名
void chusrname() {
    // 不允许修改admin用户
    if(!strcmp(current_user, "admin")) {
        printf("Can't change admin!\n");
        return;
    }
    for(int i = 0; i < user_num; i++) {
        if(!strcmp(current_user, User[i].username)) {
            printf("Please input the new name of the user: ");
            char tmp[20];
            scanf("%s", tmp);

            // 检查用户名是否已存在
            for(int j = 0; j < user_num; j++) {
                if(!strcmp(tmp, User[j].username)) {
                    printf("The username has already existed!\n");
                    return;
                }
            }

            strcpy(User[i].username, tmp);

            // 重新写入userlist.txt
            FILE* fp = fp = fopen("./userlist.txt", "w");
            if (fp == NULL) {
                printf("Can't open userlist.txt!\n");
                exit(0);
            }
            for(int j = 0; j < user_num; j++) {
                fprintf(fp, "%s %s %s\n", User[j].username, User[j].password, User[j].disk_name);
            }
            fclose(fp);

            strcpy(current_user, User[i].username);

            strcpy(current_path, "[");
            strcat(current_path, current_user);
            strcat(current_path, "@ext2 /");

            printf("Change username successfully!\n");
            return;
        }
    }
}

// 修改用户密码
void chusrpwd() {
    for(int i = 0; i < user_num; i++) {
        if(!strcmp(current_user, User[i].username)) {
            printf("Please input the new password of the user: ");
            scanf("%s", User[i].password);

            // 重新写入userlist.txt
            FILE* fp = fp = fopen("./userlist.txt", "w");
            if (fp == NULL) {
                printf("Can't open userlist.txt!\n");
                exit(0);
            }
            for(int j = 0; j < user_num; j++) {
                fprintf(fp, "%s %s %s\n", User[j].username, User[j].password, User[j].disk_name);
            }
            fclose(fp);

            printf("Change password successfully!\n");
            return;
        }
    }
}

// 查看指令
void help() {
    printf("   ---------------------------------------------------------------------------\n");
    printf("                             Available Instructions                           \n");
    printf("    1.cd    2.ls    3.create   4.mkdir   5.rm    6.rmdir    7.open    8.close \n");
    printf("    9.read  10.write   11.chmod   12.format   13.ckdisk  14.help  15.quit     \n");
    printf("    16.mkusr   17.rmusr   18.chusrname  19.chusrpwd  20.chusr                 \n");
    printf("   ---------------------------------------------------------------------------\n");
}

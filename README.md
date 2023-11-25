# EXT2-FILE-SYSTEM
操作系统实验，模拟实现EXT2文件系统，初始用户为admin，密码为admin，输入help查看支持的指令。为简单起见，逻辑块大小与物理块大小均定义为512字节，且一个用户只定义一个组。组描述符只占用一个块。引导块和superblock块省略，superblock块功能由组描述符块代替。

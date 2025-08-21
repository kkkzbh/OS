

#ifndef _IO_H
#define _IO_H

#include <cdefs.h>

__BEGIN_DECLS

#include <stdint.h>

void static outb(u16 port,u8 data)
{
    /*
     * 向端口 port 写入一个字节
     */
    asm volatile(
        "outb %b0, %w1" : : "a"(data), "Nd"(port)
    );
}

void static outsw(u16 port,void const* addr,u32 word_cnt)
{
    /*
     * 把 ds:esi 处的16位的内容写入port端口
     */
    asm volatile(
        "cld; rep outsw" : "+S"(addr), "+c"(word_cnt) : "d"(port)
    );
}

u8 static inb(u16 port)
{
    /*
     * 从端口port读取一个字节返回
     */
    u8 data;
    asm volatile(
        "inb %w1, %b0" : "=a"(data) : "Nd"(port)
    );
    return data;
}

void static insw(u16 port,void* addr,u32 word_cnt)
{
    /*
     * 从port处不断读入的16位内容写入 es:edi 指向的内存
     */
    asm volatile(
        "cld; rep insw" : "+D"(addr),"+c"(word_cnt) : "d"(port) : "memory"
    );
}

__END_DECLS

#endif //_IO_H

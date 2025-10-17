

#include <stdio.h>
#include <interrupt.h>
#include <io.h>
#include <global.h>
#include <keyboard.h>

auto constexpr KBD_BUF_PORT = 0x60;     // 键盘buffer 寄存器端口号 0x60

/* 用转义字符定义部分控制字符 */
auto constexpr esc              = '\033';   // 八进制表示字符，也可以用十六进制'\x1b'
auto constexpr backspace        = '\b';
auto constexpr tab              = '\t';
auto constexpr enter            = '\r';
auto constexpr delete_key       = '\177';   // 八进制表示字符，十六进制为'\x7f'

/* 以上不可见字符一律定义为 0 */
auto constexpr char_invisible   = 0;
auto constexpr ctrl_l_char      = char_invisible;
auto constexpr ctrl_r_char      = char_invisible;
auto constexpr shift_l_char     = char_invisible;
auto constexpr shift_r_char     = char_invisible;
auto constexpr alt_l_char       = char_invisible;
auto constexpr alt_r_char       = char_invisible;
auto constexpr caps_lock_char   = char_invisible;

/* 定义控制字符的通码和断码 */
auto constexpr shift_l_make     = 0x2a;
auto constexpr shift_r_make     = 0x36;
auto constexpr alt_l_make       = 0x38;
auto constexpr alt_r_make       = 0xe038;
auto constexpr alt_r_break      = 0xe0b8;
auto constexpr ctrl_l_make      = 0x1d;
auto constexpr ctrl_r_make      = 0xe01d;
auto constexpr ctrl_r_break     = 0xe09d;
auto constexpr caps_lock_make   = 0x3a;

/* 定义以下变量记录相应键按下的状态，
 * ext_scancode 用于记录 makecode 是否以 0xe0 开头 */
bool static ctrl_status, shift_status, alt_status, caps_lock_status, ext_scancode;

/* 以通码make_code为索引的二维数组 */
char static constexpr keymap[][2] = {
/* 扫描码   未与shift组合  与shift组合*/
/* 0x00 */	{0,	0},
/* 0x01 */	{esc,	esc},
/* 0x02 */	{'1',	'!'},
/* 0x03 */	{'2',	'@'},
/* 0x04 */	{'3',	'#'},
/* 0x05 */	{'4',	'$'},
/* 0x06 */	{'5',	'%'},
/* 0x07 */	{'6',	'^'},
/* 0x08 */	{'7',	'&'},
/* 0x09 */	{'8',	'*'},
/* 0x0A */	{'9',	'('},
/* 0x0B */	{'0',	')'},
/* 0x0C */	{'-',	'_'},
/* 0x0D */	{'=',	'+'},
/* 0x0E */	{backspace, backspace},
/* 0x0F */	{tab,	tab},
/* 0x10 */	{'q',	'Q'},
/* 0x11 */	{'w',	'W'},
/* 0x12 */	{'e',	'E'},
/* 0x13 */	{'r',	'R'},
/* 0x14 */	{'t',	'T'},
/* 0x15 */	{'y',	'Y'},
/* 0x16 */	{'u',	'U'},
/* 0x17 */	{'i',	'I'},
/* 0x18 */	{'o',	'O'},
/* 0x19 */	{'p',	'P'},
/* 0x1A */	{'[',	'{'},
/* 0x1B */	{']',	'}'},
/* 0x1C */	{enter,  enter},
/* 0x1D */	{ctrl_l_char, ctrl_l_char},
/* 0x1E */	{'a',	'A'},
/* 0x1F */	{'s',	'S'},
/* 0x20 */	{'d',	'D'},
/* 0x21 */	{'f',	'F'},
/* 0x22 */	{'g',	'G'},
/* 0x23 */	{'h',	'H'},
/* 0x24 */	{'j',	'J'},
/* 0x25 */	{'k',	'K'},
/* 0x26 */	{'l',	'L'},
/* 0x27 */	{';',	':'},
/* 0x28 */	{'\'',	'"'},
/* 0x29 */	{'`',	'~'},
/* 0x2A */	{shift_l_char, shift_l_char},
/* 0x2B */	{'\\',	'|'},
/* 0x2C */	{'z',	'Z'},
/* 0x2D */	{'x',	'X'},
/* 0x2E */	{'c',	'C'},
/* 0x2F */	{'v',	'V'},
/* 0x30 */	{'b',	'B'},
/* 0x31 */	{'n',	'N'},
/* 0x32 */	{'m',	'M'},
/* 0x33 */	{',',	'<'},
/* 0x34 */	{'.',	'>'},
/* 0x35 */	{'/',	'?'},
/* 0x36 */	{shift_r_char, shift_r_char},
/* 0x37 */	{'*',	'*'},
/* 0x38 */	{alt_l_char, alt_l_char},
/* 0x39 */	{' ',	' '},
/* 0x3A */	{caps_lock_char, caps_lock_char}
/*其它按键暂不处理*/
};

// 键盘中断处理函数 (状态机)
void static intr_keyboard_handler()
{
    auto ctrl_down_last = ctrl_status;
    auto shift_down_last = shift_status;
    auto caps_lock_last = caps_lock_status;

    auto scancode = (u16)inb(KBD_BUF_PORT);  // 读取输出缓冲寄存器，否则8042不再继续响应键盘中断

    // e0开头表示多扫描码
    if(scancode == 0xe0) {
        ext_scancode = true;
        return;
    }

    if(ext_scancode) {
        scancode |= 0xe000;
        ext_scancode = false;
    }

    auto break_code = (bool)(scancode & 0x0080);

    // 如果是断码
    if(break_code) {
        // ctrl_r 与 alt_r 断码和通码都是两个字节
        // 用以下方式取出通码
        auto make_code = (scancode &= 0xff7f);
        if(make_code == ctrl_l_make || make_code == ctrl_r_make) {
            ctrl_status = false;
        } else if(make_code == shift_l_make || make_code == shift_r_make) {
            shift_status = false;
        } else if(make_code == alt_l_make || make_code == alt_r_make) {
            alt_status = false;
        }
        // ***  caps_lock是切换式 单独处理  ***
        // 在这里忽略 caps_lock的断码
        return;
    }

    // 如果是通码
    auto shift = false; // 判断是否与shift结合
    if((scancode > 0x00 && scancode < 0x3b) || scancode == alt_r_make || scancode == ctrl_r_make) {
        if(scancode < 0x0e || scancode == 0x29 || scancode == 0x1a || scancode == 0x1b || scancode == 0x2b || scancode == 0x27 || scancode == 0x28 || scancode == 0x33 || scancode == 0x34 || scancode == 0x35) {
            // 0~9  -  =  `  [  ]  \  ;  '  ,  .  /
            if(shift_down_last) {
                shift = true;
            }
        } else { // 默认为字母键
            shift ^= shift_down_last;
            shift ^= caps_lock_last;
        }
        auto index = (u8)(scancode &= 0x00ff); // 取8位 针对高字节e0
        auto c = keymap[index][shift];
        if(ctrl_down_last && (c == 'l' || c == 'u')) {
            c -= 'a';   // 处理 ctrl+l 和 ctrl+u 的快捷键
        }
        if(c) { // 处理 ASCII 不为0的键  (所有控制字符提前处理为0)
            solve(c); // 处理 c
            return;
        }
        if(scancode == ctrl_l_make || scancode == ctrl_r_make) {
            ctrl_status = true;
        } else if(scancode == shift_l_make || scancode == shift_r_make) {
            shift_status = true;
        } else if(scancode == alt_l_make || scancode == alt_r_make) {
            alt_status = true;
        } else if(scancode == caps_lock_make) {
            // 只要按下就是取反 且前面忽略了caps_lock的弹起
            caps_lock_status = !caps_lock_status;
        }

        return;
    }

    puts("unknown key!\n");

}

// 键盘初始化函数
void keyboard_init()
{
    puts("keyboard init start\n");
    register_handler(0x21, intr_keyboard_handler);  // 注册键盘中断处理函数
}
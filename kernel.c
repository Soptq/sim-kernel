/* The following keyboard map array is taken from
    http://www.osdever.net/bkerndev/Docs/keyboard.htm
   All credits where due
*/

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 28
#define SCREEN_CHAR_BYTE 2

#define ENTER_KEY_CODE 0x1C
#define BACKSPACE_KEY_CODE 0x0E
#define SYS_INPUT_HINT "-> "

unsigned char keyboard_map[128] =
        {
                0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
                '9', '0', '-', '=', '\b',	/* Backspace */
                '\t',			/* Tab */
                'q', 'w', 'e', 'r',	/* 19 */
                't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
                0,			/* 29   - Control */
                'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
                '\'', '`',   0,		/* Left shift */
                '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
                'm', ',', '.', '/',   0,				/* Right shift */
                '*',
                0,	/* Alt */
                ' ',	/* Space bar */
                0,	/* Caps lock */
                0,	/* 59 - F1 key ... > */
                0,   0,   0,   0,   0,   0,   0,   0,
                0,	/* < ... F10 */
                0,	/* 69 - Num lock*/
                0,	/* Scroll Lock */
                0,	/* Home key */
                0,	/* Up Arrow */
                0,	/* Page Up */
                '-',
                0,	/* Left Arrow */
                0,
                0,	/* Right Arrow */
                '+',
                0,	/* 79 - End key*/
                0,	/* Down Arrow */
                0,	/* Page Down */
                0,	/* Insert Key */
                0,	/* Delete Key */
                0,   0,   0,
                0,	/* F11 Key */
                0,	/* F12 Key */
                0,	/* All other keys are undefined */
        };

struct IDT_entry{
    unsigned short int offset_lowerbits;        /* 16 bits */
    unsigned short int selector;                /* 16 bits */
    unsigned char zero;                         /* 8 bits */
    unsigned char type_attr;                    /* 8 bits */
    unsigned short int offset_higherbits;       /* 16 bits */
};

extern void keyboard_handler(void);
extern char read_port(unsigned short port);     /* the return value is stored in EAX, which is 32 bits */
extern void write_port(unsigned short port, unsigned char data);
extern void load_idt(unsigned long *idt_ptr);

/* 256 interrupts */
struct IDT_entry IDT[256];

/* video memory starting at 0xb8000, supporting 25 lines, each line contains 80 ascii characters */
char *vidptr = (char*)0xb8000;

/* current cursor location */
unsigned int current_loc = 0;

/* vid cache buffer */
unsigned char vidcache[SCREEN_CHAR_BYTE * SCREEN_WIDTH * SCREEN_HEIGHT];
unsigned int vidcache_loc = 0;

/* empty the video cache buffer */
void clear_vidcache() {
    vidcache[0] = '\0';
    vidcache_loc = 0;
}

void push_vidcache(unsigned char c) {
    vidcache[vidcache_loc++] = c;
}

unsigned int get_charlen(const char *str) {
    unsigned int i = 0;
    while (str[i++] != '\0');
    return i;
}

int cmp_str(const char *str) {
    unsigned int vidcache_len = get_charlen(vidcache);
    unsigned int str_len = get_charlen(str);

    if (vidcache_len != str_len)
        return 0;

    for (unsigned int i = 0; i < str_len; ++i) {
        if (str[i] != vidcache[i])
            return 0;
    }

    return 1;
}

void idt_init(void) {
    unsigned long keyboard_address;
    unsigned long idt_address;
    unsigned long idt_ptr[2];

    keyboard_address = (unsigned long)keyboard_handler; /* this is where our interrupt handler located */
    IDT[0x21].offset_lowerbits = keyboard_address & 0xffff;
    IDT[0x21].selector = 0x08;  /* GRUB provided a GDT, and the descriptor is the second 8-bits arguments. */
    IDT[0x21].zero = 0;
    IDT[0x21].type_attr = 0x8e;
    IDT[0x21].offset_higherbits = (keyboard_address & 0xffff0000) >> 16;

    /*              Ports
     *              PIC1    PIC2
     * Command      0x20    0xA0
     * Data         0x21    0xA1
     * */

    /* ICW1, check 8259 PIC bits instructions */
    write_port(0x20, 0x11);
    write_port(0xA0, 0x11);

    /* ICW2, interrupts vectors. Avoid first 32 bits as they are reserved */
    write_port(0x21, 0x20);
    write_port(0xA1, 0x28);

    /* ICW3, relations of PICs */
    write_port(0x21, 0x00);
    write_port(0xA1, 0x00);

    /* ICW4, additional settings */
    write_port(0x21, 0x01);
    write_port(0xA1, 0x01);

    /* IMR */
    write_port(0x21, 0xff);
    write_port(0x21, 0xff);

    idt_address = (unsigned long)IDT;
    idt_ptr[0] = (sizeof(struct IDT_entry) * 256) + ((idt_address & 0xffff) << 16); /* offset */
    idt_ptr[1] = idt_address >> 16; /* segment */

    load_idt(idt_ptr);
}

/* init keyboard. unmask the IMR */
void kb_init(void) {
    /* 11111101 => 0xFD */
    write_port(0x21, 0xfd);
}

void kprint_newline(void){
    current_loc = current_loc + (SCREEN_CHAR_BYTE * SCREEN_WIDTH - current_loc % SCREEN_WIDTH);
}

void kprint(const char *str, unsigned short attr) {
    unsigned int i = 0;
    while (str[i] != '\0') {
        push_vidcache(str[i]);
        vidptr[current_loc++] = str[i++];
        vidptr[current_loc++] = attr;
    }
}

void kprint_char(const unsigned char str, unsigned short attr) {
    push_vidcache(str);
    vidptr[current_loc++] = str;
    vidptr[current_loc++] = attr;
}

void kprint_sys(const char *str, unsigned short attr) {
    unsigned int i = 0;
    while (str[i] != '\0') {
        vidptr[current_loc++] = str[i++];
        vidptr[current_loc++] = attr;
    }
}

void kprintln_sys(const char *str, unsigned short attr) {
    kprint_sys(str, attr);
    kprint_newline();
}

/* clar the video buffer ( clear the screen )
     * each ascii character takes 2 bytes
     * the first byte is used to store character in ascii
     * the second byte is used to store character's attribute like color
     * */
void clear_screen(void) {
    unsigned int i = 0;
    while (i < SCREEN_HEIGHT * SCREEN_WIDTH * SCREEN_CHAR_BYTE) {
        vidptr[i++] = ' ';
        vidptr[i++] = 0x07;
    }
    current_loc = 0;
    clear_vidcache();
}

void command_handler(void) {
    push_vidcache('\0');
    kprint_newline();
    if (cmp_str("clear")) {
        clear_screen();
    } else if (cmp_str("help")) {
        kprintln_sys("clear: clear the screen.", 0x07);
        kprintln_sys("help: show all commands.", 0x07);
    }
    clear_vidcache();
}

void kbackspace(void) {
    if (current_loc % SCREEN_WIDTH >= get_charlen(SYS_INPUT_HINT) * SCREEN_CHAR_BYTE) {
        vidptr[--current_loc] = 0x07;
        vidptr[--current_loc] = ' ';
    }
}

/* our interrupt handler, this will be invoked from assemble */
void keyboard_handler_main(void){
    /* keyboard has two I/O ports, one can read the status and the other can read the keycode */
    unsigned char status;
    char keycode;

    /* notify PIC that this interrupt is being acknowledged */
    write_port(0x20, 0x20);

    status = read_port(0x64);   /* 0x64 is the keyboard status port */
    if (status & 0x01) {    /* if the first bit is set, then the keyboard output buffer is full */
        keycode = read_port(0x60);  /* 0x60 is the keyboard data port */
        if (keycode < 0)
            return; /* something must goes wrong, abort this. */

        if (keycode == ENTER_KEY_CODE) {
            command_handler();
            kprint_sys(SYS_INPUT_HINT, 0x02);
            return;
        }

        if (keycode == BACKSPACE_KEY_CODE) {
            kbackspace();
            return;
        }

        kprint_char(keyboard_map[(unsigned char) keycode], 0x07);
    }
}

void kernel_start(void) {
    clear_screen();
    kprint_sys("Soptq's micro kernel v1.0", 0x02);
    kprint_newline();
    kprint_sys("Comopiled on 7.18.2020", 0x02);
    kprint_newline();
    kprint_newline();
    kprint_sys(SYS_INPUT_HINT, 0x02);

    idt_init();
    kb_init();

    while (1);
}

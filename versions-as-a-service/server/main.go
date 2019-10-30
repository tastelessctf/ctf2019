package main

import (
	"context"
	"errors"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"net"
	"os"
	"path/filepath"
	"time"

	"google.golang.org/grpc"
	"tasteless.eu/ctf/a2019/vaas"
)

var (
	loggedError = func(uid, gid, cid uint64, a string, err error) error {
		log.Printf("user: %d/%d@%d, msg: %s, err: %s", uid, gid, cid, a, err)
		return fmt.Errorf("user: %d/%d@%d, msg: %s, err: %w", uid, gid, cid, a, err)
	}

	errEnterprise = errors.New("please purchase VAAS enterprise to enable this feature")
)

func check(err error) {
	if err != nil {
		log.Fatal(err)
	}
}

type vaaServiceServer struct {
	uid, gid, cid uint64
	revCount      int
	revmsg        [10]string
}

func (v *vaaServiceServer) Commit(_ context.Context, r *vaas.CommitRequest) (*vaas.CommitResponse, error) {
	if len(r.GetContent()) > 1<<20 {
		return nil, loggedError(v.uid, v.gid, v.cid, "commit: file too big!", errEnterprise)
	}

	if len(r.GetMessage()) > 1<<10 {
		return nil, loggedError(v.uid, v.gid, v.cid, "commit: message too big!", errEnterprise)
	}

	if v.revCount >= 10 {
		return nil, loggedError(v.uid, v.gid, v.cid, "commit: maximum revisions reached!", errEnterprise)
	}

	revname := fmt.Sprintf("rev%d", v.revCount)
	v.revmsg[v.revCount] = r.GetMessage()
	v.revCount++

	err := ioutil.WriteFile(filepath.Join("revs", revname), r.GetContent(), 0400)
	return &vaas.CommitResponse{Version: revname}, loggedError(v.uid, v.gid, v.cid, "WriteFile", err)
}

func (v *vaaServiceServer) Diff(_ context.Context, r *vaas.DiffRequest) (*vaas.DiffResponse, error) {
	return nil, loggedError(v.uid, v.gid, v.cid, "diff: disabled!", errEnterprise)
}

func (v *vaaServiceServer) Apply(_ context.Context, r *vaas.ApplyRequest) (*vaas.ApplyResponse, error) {
	f, err := os.OpenFile(filepath.Join("revs", r.GetRevision()), os.O_WRONLY, 0)
	if err != nil {
		return nil, loggedError(v.uid, v.gid, v.cid, "OpenFile", err)
	}

	defer f.Close()

	status := "patching " + r.GetRevision() + "."

	for _, patch := range r.GetPatches() {
		n, err := f.WriteAt(patch.GetPatch(), patch.GetOffset())
		if err != nil {
			return nil, loggedError(v.uid, v.gid, v.cid, "WriteAt", err)
		}

		status += fmt.Sprintf("patched %d at 0x%x.", n, patch.GetOffset())
	}

	return &vaas.ApplyResponse{Status: status}, nil
}

func (v *vaaServiceServer) Checkout(_ context.Context, r *vaas.CheckoutRequest) (*vaas.CheckoutResponse, error) {
	b, err := ioutil.ReadFile(filepath.Join("revs", filepath.Clean(r.GetRevision())))
	if err != nil {
		return nil, err
	}

	return &vaas.CheckoutResponse{Content: b}, nil
}

func (v *vaaServiceServer) Identify(_ context.Context, r *vaas.IdentifyRequest) (*vaas.IdentifyResponse, error) {
	v.uid = r.GetUid()
	v.gid = r.GetGid()
	v.cid = r.GetCid()
	return &vaas.IdentifyResponse{Status: fmt.Sprintf("Identified User %d in Group %d on Computer %d", v.uid, v.gid, v.cid)}, nil
}

type ioListener struct {
	in   io.Reader
	out  io.Writer
	open bool
}

func (l *ioListener) Accept() (net.Conn, error) {
	if l.open {
		time.Sleep(1000 * time.Second)
		return nil, nil
	}
	l.open = true
	return l, nil
}

func (*ioListener) Close() error { return nil }
func (l *ioListener) Addr() net.Addr {
	return l
}

func (*ioListener) Network() string {
	return "io"
}

func (l *ioListener) String() string {
	return fmt.Sprintf("%s/%s", l.in, l.out)
}

func (l *ioListener) Read(b []byte) (n int, err error) {
	return l.in.Read(b)
}

func (l *ioListener) Write(b []byte) (n int, err error) {
	return l.out.Write(b)
}

func (l *ioListener) LocalAddr() net.Addr { return l }

func (l *ioListener) RemoteAddr() net.Addr { return l }

func (l *ioListener) SetDeadline(t time.Time) error { return nil }

func (l *ioListener) SetReadDeadline(t time.Time) error { return nil }

func (l *ioListener) SetWriteDeadline(t time.Time) error { return nil }

func main() {
	check(os.MkdirAll("revs", 0700))

	log.SetFlags(log.Ltime | log.Lshortfile)

	// listener, err := net.Listen("tcp", ":12345")
	// check(err)
	listener := &ioListener{
		in:  os.Stdin,
		out: os.Stdout,
	}

	server := grpc.NewServer()

	vaas.RegisterVaaServiceServer(server, new(vaaServiceServer))

	// reflection.Register(server)

	check(server.Serve(listener))
}

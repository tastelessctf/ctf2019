package main

import (
	"crypto/ecdsa"
	"crypto/elliptic"
	"fmt"
	"io"
	"log"
	"math/big"
	"net"
	"net/http"
	"strings"
	"sync"

	"github.com/dgrijalva/jwt-go"
)

var (
	px  = new(big.Int)
	py  = new(big.Int)
	pub *ecdsa.PublicKey
)

func init() {
	px.UnmarshalText([]byte("93251906037234108648884401545082428994204637700185511215250769523856109874824"))
	py.UnmarshalText([]byte("113225164272001374099967346704706720034439091542943192365944987666233917082633"))

	pub = &ecdsa.PublicKey{
		Curve: elliptic.P256(),
		X:     px,
		Y:     py,
	}
}

type handler struct {
	internal bool
}

func pipe(left, right io.ReadWriter) {
	wg := new(sync.WaitGroup)
	wg.Add(2)

	go func() {
		io.Copy(left, right)
		wg.Done()
		if c, ok := left.(io.Closer); ok {
			c.Close()
		}
		if c, ok := right.(io.Closer); ok {
			c.Close()
		}
	}()

	go func() {
		io.Copy(right, left)
		wg.Done()
		if c, ok := left.(io.Closer); ok {
			c.Close()
		}
		if c, ok := right.(io.Closer); ok {
			c.Close()
		}
	}()

	wg.Wait()
}

var whitelisted = map[string]map[string]struct{}{
	"auth.corp": {
		"/login": struct{}{},
	},
}

func allowed(r *http.Request) bool {
	if wl, ok := whitelisted[r.Host]; ok {
		if _, ok := wl[r.URL.Path]; ok {
			return true
		}
	}

	token, err := r.Cookie("auth")
	if err != nil {
		return false
	}

	t, err := jwt.Parse(token.Value, func(token *jwt.Token) (interface{}, error) {
		if _, ok := token.Method.(*jwt.SigningMethodECDSA); !ok {
			return nil, fmt.Errorf("Unexpected signing method: %v", token.Header["alg"])
		}

		return pub, nil
	})
	if err != nil {
		return false
	}

	return t.Valid
}

func (h *handler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	// only allow corporate hosts
	hostname := strings.Split(r.Host, ":")[0]
	if !strings.HasSuffix(hostname, ".corp") {
		http.NotFound(w, r)
		return
	}

	// check token
	if !allowed(r) {
		http.Redirect(w, r, "http://auth.corp/login", http.StatusSeeOther)
		return
	}

	// allow internal connects
	if h.internal {
		if r.Method == http.MethodConnect {
			n, _ := net.Dial("tcp", r.RequestURI)
			w.WriteHeader(200)

			_, b, _ := w.(http.Hijacker).Hijack()

			pipe(n, b)
			return
		}
		http.Error(w, "internal interface", http.StatusBadGateway)
		return
	}

	// connect to backend (default to port 80)
	if !strings.Contains(r.Host, ":") {
		r.Host += ":80"
	}
	n, err := net.Dial("tcp", r.Host)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	// send original request
	r.Write(n)

	// stream raw TCP connection, so websockets etc. work too
	c, b, err := w.(http.Hijacker).Hijack()
	pipe(n, b)
	c.Close()
}

func main() {
	go func() {
		log.Fatal(http.ListenAndServe(":80", &handler{internal: true}))
	}()
	log.Fatal(http.ListenAndServe(":8080", &handler{}))
}

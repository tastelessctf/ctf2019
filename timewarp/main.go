package main

import (
	"bytes"
	"crypto/ed25519"
	"crypto/rand"
	"encoding/base64"
	"fmt"
	"io/ioutil"
	"log"
	"net/http"
	"os"
	"strings"
	"time"
)

var (
	loc, _ = time.LoadLocation("Europe/Berlin")
	pub    ed25519.PublicKey
	priv   ed25519.PrivateKey
	flag   = os.Getenv("FLAG")
)

func init() {
	k, err := ioutil.ReadFile("/key/key.dat")
	if err != nil {
		log.Println(err)
		pub, priv, _ = ed25519.GenerateKey(rand.Reader)
		log.Println(ioutil.WriteFile("/key/key.dat", bytes.Join([][]byte{pub, priv}, []byte(":")), 0600))
		return
	}

	ks := bytes.Split(k, []byte(":"))
	pub, priv = ks[0], ks[1]
}

func sendLine(w http.ResponseWriter, line string) {
	flusher, _ := w.(http.Flusher)

	fmt.Fprintf(w, "%s<br/>", line)
	flusher.Flush()
	time.Sleep(1 * time.Second)
}

func timewarpHandler(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "text/html; charset=utf-8")

	sendLine(w, "It's just a jump to the left")
	sendLine(w, "And then a step to the right")
	sendLine(w, "With your hands on your hips")
	sendLine(w, "You bring your knees in tight")
	sendLine(w, "But it's the pelvic thrust")
	sendLine(w, "That really drives you insane")
	sendLine(w, "Let's do the Time Warp again")

	fmt.Fprintf(w, "<br/><br/>")

	if r.FormValue("token") == "" {
		fmt.Fprintf(w, "oh no! where is your ?token= ?")
		return
	}

	token := strings.Split(r.FormValue("token"), ".")
	if len(token) != 3 {
		fmt.Fprintf(w, "oh no! what happened to your token?")
		return
	}

	sig, err := base64.URLEncoding.DecodeString(token[2])
	if err != nil {
		fmt.Fprintf(w, "oh no! all your base64 belongs to us!")
		return
	}

	if !ed25519.Verify(pub, []byte(token[0]+"."+token[1]), sig) {
		fmt.Fprintf(w, "oh no! what happened to your integrity?")
		return
	}

	ts, err := base64.URLEncoding.DecodeString(token[1])
	if err != nil {
		fmt.Fprintf(w, "this should not have happened: %s", err.Error())
		return
	}

	t, err := time.ParseInLocation(time.ANSIC, string(ts), loc)
	if err != nil {
		fmt.Fprintf(w, "this should not have happened: %s", err.Error())
		return
	}

	// we need to parse our own "now" time as well, otherwise this would cause any token issues during the timeshift to be valid
	now, err := time.ParseInLocation(time.ANSIC, time.Now().Add(5*time.Second).In(loc).Format(time.ANSIC), loc)
	if err != nil {
		fmt.Fprintf(w, "this should not have happened: %s", err.Error())
		return
	}

	if t.Before(now) {
		fmt.Fprint(w, "oh no! too slow! your token is not valid anymore :(")
		return
	}

	fmt.Fprintf(w, "faster than time, hu? here is your flag: %s", flag)
}

func tokenHandler(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "text/html; charset=utf-8")

	sendLine(w, "It's astounding;")
	sendLine(w, "Time is fleeting;")
	sendLine(w, "Madness takes its toll.")
	sendLine(w, "But listen closely...")

	enc := func(s string) string {
		return base64.URLEncoding.EncodeToString([]byte(s))
	}

	token := enc("giveFlag/Europe/Berlin") + "." + enc(time.Now().Add(5*time.Second).In(loc).Format(time.ANSIC))

	token += "." + enc(string(ed25519.Sign(priv, []byte(token))))

	fmt.Fprintf(w, `<br/><br/>here is your token (valid for 5 seconds): <a href="/timewarp?token=%s">%s</a>`, token, token)
}

func indexHandler(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "text/html; charset=utf-8")

	fmt.Fprintf(w, `go <a href="/token">grab a token</a> and <a href="/timewarp">let's dance</a>!`)
}

func main() {
	http.HandleFunc("/timewarp", timewarpHandler)
	http.HandleFunc("/token", tokenHandler)
	http.HandleFunc("/", indexHandler)

	log.Fatal(http.ListenAndServe(":8765", nil))
}

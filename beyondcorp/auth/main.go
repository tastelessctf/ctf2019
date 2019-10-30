package main

import (
	"crypto/ecdsa"
	"crypto/elliptic"
	"fmt"
	"log"
	"math/big"
	"net/http"

	"github.com/dgrijalva/jwt-go"
)

var (
	px   = new(big.Int)
	py   = new(big.Int)
	pd   = new(big.Int)
	priv *ecdsa.PrivateKey
)

func init() {
	px.UnmarshalText([]byte("93251906037234108648884401545082428994204637700185511215250769523856109874824"))
	py.UnmarshalText([]byte("113225164272001374099967346704706720034439091542943192365944987666233917082633"))
	pd.UnmarshalText([]byte("44609201853334486427617789244911256190490581444485473898784989090395955711826"))

	priv = &ecdsa.PrivateKey{
		PublicKey: ecdsa.PublicKey{
			Curve: elliptic.P256(),
			X:     px,
			Y:     py,
		},
		D: pd,
	}
}

func register(w http.ResponseWriter, r *http.Request) {
	if r.FormValue("user") == "" {
		fmt.Fprintf(w, `Please specify "user"`)
		return
	}

	t, err := jwt.NewWithClaims(jwt.SigningMethodES256, jwt.StandardClaims{
		Subject: r.FormValue("user"),
	}).SignedString(priv)

	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	fmt.Fprintf(w, `your token: %q`, t)
}

func login(w http.ResponseWriter, r *http.Request) {
	if r.Method == http.MethodPost {
		fmt.Fprintf(w, `Invalid credentials.`)
		return
	}

	fmt.Fprintf(w, `<form method="POST">Username: <input/><br/>Password: <input/><br/><input type="submit"/></form>`)

	// t, err := jwt.Parse(token, func(token *jwt.Token) (interface{}, error) {
	// 	if _, ok := token.Method.(*jwt.SigningMethodECDSA); !ok {
	// 		return nil, fmt.Errorf("Unexpected signing method: %v", token.Header["alg"])
	// 	}

	// 	return priv.Public(), nil
	// })
	// if err != nil {
	// 	return err
	// }

	// // t...
	// _ = t

	// return nil
}

func main() {
	http.HandleFunc("/login", login)
	http.HandleFunc("/register", register)

	log.Fatal(http.ListenAndServe(":80", nil))
}

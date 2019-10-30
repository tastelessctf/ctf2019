package main

import "net/http"

func main() {
	http.ListenAndServe(":80", http.FileServer(http.Dir("content")))
}

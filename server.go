package main

// Simple http server listening on IP
// of your TUN/TAP device

import (
	"fmt"
	"log"
	"net/http"
)

func hello(w http.ResponseWriter, req *http.Request) {
	fmt.Fprintf(w, "hello\n")
}

func main() {
	http.HandleFunc("/", hello)
	address := "11.0.0.1:8000"
	fmt.Printf("Server attempting to listening on %v\n", address)
	err := http.ListenAndServe(address, nil)
	if err != nil {
		log.Fatal(err)
	}
}

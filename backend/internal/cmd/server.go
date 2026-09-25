package cmd

import (
	"net/http"

	"github.com/go-chi/chi/v5"
)

func RunServer() {
	r := chi.NewRouter()
	r.Get("/", func(w http.ResponseWriter, r *http.Request) {
		w.Write([]byte("welcome!\n"))
	})
	http.ListenAndServe(":8080", r)
}

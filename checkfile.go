package main

import (
	"fmt"
	"io"
	"log"
	"os"
	"strconv"
	"strings"
)

func main() {
	b, err := io.ReadAll(os.Stdin)
	if err != nil {
		log.Fatal(err)
	}
	lines := strings.Split(string(b), "\n")
	for no, l := range lines {
		if len(l) == 0 {
			fmt.Printf("line %d is empty\n", no)
			continue
		}
		if l[0] == '#' {
			continue
		}
		f := strings.Fields(l)
		if len(f) != 2 {
			fmt.Printf("line %d has %d fields, not 2\n", no, len(f))
			continue
		}
		for s, fld := range f {
			if _, err := strconv.Atoi(fld); err != nil {
				fmt.Printf("Line %d: field %d(%q) is not a number: %v", no, s, fld, err)
			}
		}
	}
}

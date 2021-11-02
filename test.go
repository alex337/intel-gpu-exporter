package main

//#cgo LDFLAGS: -L /usr/lib -lsgm_monitor
/*
#include "m.c"
int display();
*/
import "C"

func main(){
	C.display()
}


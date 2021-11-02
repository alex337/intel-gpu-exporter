package main
//#cgo LDFLAGS: -L /usr/lib -lsgm_monitor
/*
#include "sg_monitor.h"
*/
import "C"


//type CardInfo struct {
//	Type            string
//	Status          string
//	CardReserved    string
//}
//
//type NodeInfo struct {
//	Type              string
//	Status            string
//	PciAddr           string
//	GPUIndex          uint
//	GPUGen            uint
//	GPUEuCount        uint
//	GPUBaseFreq       uint
//	GPUTurboFreq      uint
//	GPUVramSize       uint
//	GPUSliceCount     uint
//	GPUSubsliceCount  uint
//	GPUVdboxCount     uint
//	GPUL3Size         uint
//	NodeReserved      [9]uint
//}
//
//type GPUDevice struct{ dev C.sgm_card_t }
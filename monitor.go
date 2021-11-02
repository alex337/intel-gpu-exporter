package main


//#cgo LDFLAGS: -L /usr/lib -lsgm_monitor
/*
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include "sg_monitor.h"
*/
import "C"

import (
	"log"
	"strconv"
	"unsafe"
)

type CardInfo struct {
	Type            string
	Status          string
	CardReserved    string
}

type NodeInfo struct {
	Type              string
	Status            string
	PciAddr           string
	GPUIndex          uint
	GPUGen            uint
	GPUEuCount        uint
	GPUBaseFreq       uint
	GPUTurboFreq      uint
	GPUVramSize       uint
	GPUSliceCount     uint
	GPUSubsliceCount  uint
	GPUVdboxCount     uint
	GPUL3Size         uint
}

type NodeUsage struct {
	RenderFreq         float32
	RenderUsage        float32
	VdeoFreq           float32
	VideoUsage         float32
	VeUsage            float32
	BlitterFreq        float32
	BlitterUsage       float32
	MemCreated         float32
	MemImported        float32
	MemUsageGpuSys     float32
	MemUsageGpuVram    float32
	Power              float32
	Temperature        float32
}

type ProcessUsage struct {
	Pid                       uint32
	ProcessName               string
	RenderUsage               float32
	VideoUsage                float32
	VeUsage                   float32
	BlitterFreq               float32
	BlitterUsage              float32
	MemUsageCreated           float32
	MemUsageImported          float32
}

type ContainerUsage struct {
	Cid                       uint
	RenderUsage               float32
	VideoUsage                float32
	VeUsage                   float32
	BlitterFreq               float32
	BlitterUsage              float32
	MemUsageCreated           float32
	MemUsageImported          float32
	ProcessID                 []uint32
}

type GPUDevice struct{ card C.sgm_card_t }

type NodeDevice struct{node C.sgm_node_t; card C.sgm_card_t}


func Init()  {
	if C.sgm_init() == C.SGM_SUCESS{
		log.Println("Init Success")
	}else if C.sgm_init() == C.SGM_ERROR_INVALID_OPERATION {
		log.Fatalf("Not Run as root, euid: %d, please run as root\n", C.geteuid())
	} else {
		log.Fatalf("can't find gpu node, Please confirm whether the card is normal\n")
	}
}

func SgmQueryCardsCount() int{
	var numCards C.uint
	C.sgm_query_cards(&numCards)
	return int(numCards)
}

func SgmQueryCardByIndex(idx uint) GPUDevice {
	var dev C.sgm_card_t
	C.sgm_query_card_by_index(C.uint(idx), &dev)
	return GPUDevice{dev}
}

func (g *GPUDevice)SgmQueryCardInfoByIndex() *CardInfo {
	var cardInfo C.sgm_card_info_t

	C.sgm_query_card_info(g.card, &cardInfo)
	log.Println("status:", C.GoString(&cardInfo.status[0]))

	return &CardInfo{
		 Type:         C.GoString(&cardInfo._type[0]),
		 Status:       C.GoString(&cardInfo.status[0]),
		 CardReserved: C.GoString(&cardInfo.card_reserved[0]),
	}
}

func (g *GPUDevice)DeInit()  {
	C.sgm_deinit(&g.card)
}


func (g *GPUDevice)SgmQueryCardNodes() int{
	var numNodes C.uint
	C.sgm_query_card_nodes(g.card, &numNodes)
	return int(numNodes)
}

func (g *GPUDevice)SgmQueryCardNodeByIndex(idx uint) NodeDevice{
	var dev C.sgm_node_t
	C.sgm_query_card_node_by_index(g.card, C.uint(idx), &dev)
	return NodeDevice{dev, g.card}
}

func (n *NodeDevice) SgmQueryNodeInfo(cardType string) (*NodeInfo, *NodeUsage){
	//var eventCount = C.uint(0)
	var nodeInfo C.sgm_node_info_t
	//var sgmNodeEvent *C.sgm_node_event_t
	//var timeoutMs = C.uint(200)

	C.sgm_query_node_info(n.card, n.node, &nodeInfo)
	C.sgm_set_node_event_threshold(n.node, C.SGM_EVENT_TEMPERATURE_THRESHOLD1, 75)//for configurable event threshold. for testing or other needs. /temp / power / memory
	C.sgm_set_node_event_threshold(n.node, C.SGM_EVENT_TEMPERATURE_THRESHOLD2, 95)
	C.sgm_set_node_event_threshold(n.node, C.SGM_EVENT_TEMPERATURE_THRESHOLD3, 100)
	C.sgm_set_node_event_threshold(n.node, C.SGM_EVENT_POWER_THRESHOLD1, 19)
	C.sgm_set_node_event_threshold(n.node, C.SGM_EVENT_POWER_THRESHOLD2, 23)
	C.sgm_set_node_event_threshold(n.node, C.SGM_EVENT_MEMORY_THRESHOLD1, 6900)
	C.sgm_set_node_event_threshold(n.node, C.SGM_EVENT_MEMORY_THRESHOLD2, 7700)
	//C.sgm_register_node_events(n.node, 0xFFF)
	//C.sgm_listen_node_events(n.node, &eventCount, &sgmNodeEvent, timeoutMs)
	if C.strncmp(C.CString(cardType), C.CString("SG1"), C.ulong(3)) == C.int(0) || C.strncmp(C.CString(cardType), C.CString("V_SG1"), C.ulong(5)) == C.int(0) {
		return n.getNodeInfo(nodeInfo), n.getNodeUsage()
	} else {
		//C.free(n.node)
		//C.ERR("Unknown card type %s", C.CString(cardType))
		return nil,nil
	}
}
func (n *NodeDevice) getNodeInfo(nodeInfo C.sgm_node_info_t) *NodeInfo{
	if C.strncmp(&nodeInfo.status[0], C.CString("READY"), C.ulong(5)) == C.int(0){
		return &NodeInfo{
			Type:              C.GoString(&nodeInfo._type[0]),
			Status:            C.GoString(&nodeInfo.status[0]),
			PciAddr:           C.GoString(&nodeInfo.pci_addr[0]),
			GPUIndex:          uint(nodeInfo.gpu_index),
			GPUGen:            uint(nodeInfo.gpu_gen),
			GPUEuCount:        uint(nodeInfo.gpu_eu_count),
			GPUBaseFreq:       uint(nodeInfo.gpu_base_freq),
			GPUTurboFreq:      uint(nodeInfo.gpu_turbo_freq),
			GPUVramSize:       uint(nodeInfo.gpu_vram_size),
			GPUSliceCount:     uint(nodeInfo.gpu_slice_count),
			GPUSubsliceCount:  uint(nodeInfo.gpu_subslice_count),
			GPUVdboxCount:     uint(nodeInfo.gpu_vdbox_count),
			GPUL3Size:         uint(nodeInfo.gpu_l3_size),
		}
	} else {
		//C.ERR("Fail to get gpu info")
		return nil
	}
}

func (n *NodeDevice) getNodeUsage() *NodeUsage{
	var usage C.sgm_node_usage_t
	var timeoutMs = C.uint(100)
	if C.sgm_query_node_usage(n.node, &usage, timeoutMs) == C.SGM_SUCESS {
		return &NodeUsage{
			RenderFreq:         float32(usage.render_freq),
			RenderUsage:        float32(usage.render_usage),
			VdeoFreq:           float32(usage.video_freq),
			VideoUsage:         float32(usage.video_usage),
			VeUsage:            float32(usage.ve_usage),
			BlitterFreq:        float32(usage.blitter_freq),
			BlitterUsage:       float32(usage.blitter_usage),
			MemCreated:         float32(usage.mem_created),
			MemImported:        float32(usage.mem_imported),
			MemUsageGpuSys:     float32(usage.mem_usage_gpu_sys),
			MemUsageGpuVram:    float32(usage.mem_usage_gpu_vram),
			Power:              float32(usage.power),
			Temperature:        float32(usage.temperature),
		}
	} else {
		//C.ERR("Fail to get node usage\n");
		return nil
	}
}


func (g *GPUDevice)SgmQueryCardInfo(cardNum int) ([]C.sgm_card_t, []C.sgm_card_info_t) {
	cards := make([]C.sgm_card_t, cardNum)
	for i := 0; i < cardNum; i++{
		C.sgm_query_card_by_index(C.uint(i), &cards[i])
	}
	cardInfo := make([]C.sgm_card_info_t, cardNum)
	for i := 0; i < cardNum; i++{
	    C.sgm_query_card_info(cards[i], &cardInfo[i])
	    log.Println("status:", C.GoString(&cardInfo[i].status[0]))
	}
	return cards, cardInfo

}

func (n *NodeDevice) SgmGetNodeProcesses() []*ProcessUsage {
	var processesNum C.uint

	var pids *C.uint
	//var pidNameLen = C.uint(12)
	var pidName string

	//var processUsage *C.sgm_node_process_usage_t
	C.sgm_get_node_processes_num(n.node, &processesNum)
	C.sgm_get_node_processes_pids(n.node, &pids)
	// var processUsageArrayPointer *[1024]C.sgm_node_process_usage_t;
	// var puap = (**C.sgm_node_process_usage_t)(unsafe.Pointer(&processUsageArrayPointer))
	var processUsagePointer *C.sgm_node_process_usage_t

	// var processUsage = make([][1024]C.sgm_node_process_usage_t, int(processesNum))
	//var processUsage *C.sgm_node_process_usage_t

	//for i := 0; i < int(processesNum); i ++ {
	//	var p *C.sgm_node_process_usage_t
	//	processUsage[i] = p
	//}
	if int(processesNum) == 0 {
		return nil
	}
	for i := 0; i < int(processesNum); i ++ {
		C.sgm_get_node_processes_usage(n.node, processesNum, pids, &processUsagePointer, 200)
	}
	//var pppp = ([7]unsafe.Pointer)(unsafe.Pointer(processUsage[0]))
	//for i := 0; i < 7; i ++ {
	//	log.Printf(">>> %p\n", pppp[i])
	//	// log.Printf("%v\n", (*C.sgm_node_process_usage_t)(pppp[i]))
	//}
	//for i := 0; i < 7; i ++ {
	//	log.Printf("%p\n", pppp[i])
	//	log.Printf("<><>%v\n", (*C.sgm_node_process_usage_t)(pppp[i]))
	//}


	var processUsage = (*[1024]C.sgm_node_process_usage_t)(unsafe.Pointer(processUsagePointer))
	var pid =  (*[1024]C.uint)(unsafe.Pointer(pids))
	//for i := 0; i < int(processesNum); i ++ {
	//	log.Println("--------：", processUsage[i] )
	//}
	procInfo := make([]*ProcessUsage, int(processesNum))
	for i := 0; i < int(processesNum); i ++ {
		//C.sgm_get_node_processes_name(n.node, *(*C.uint)(unsafe.Pointer(pids)), pidNameLen, C.CString(pidName))
		//C.GoString(&cardInfo[i].status[0]))
		procInfo[i] = &ProcessUsage{
			Pid:                       uint32(pid[i]),
			ProcessName:               pidName,
			RenderUsage:               float32(processUsage[i].render_usage),
			VideoUsage:                float32(processUsage[i].video_usage),
			VeUsage:                   float32(processUsage[i].ve_usage),
			BlitterFreq:               float32(processUsage[i].blitter_freq),
			BlitterUsage:              float32(processUsage[i].blitter_usage),
			MemUsageCreated:           float32(processUsage[i].mem_usage_created),
			MemUsageImported:          float32(processUsage[i].mem_usage_imported),
		}
	}
	//C.free(unsafe.Pointer(pids))
	//for i := 0; i < int(processesNum); i ++ {
	//	C.free(unsafe.Pointer(processUsage[i]))
	//}
	//*C.uint(unsafe.Pointer(pids)) = C.NULL
	return procInfo
}

func (n *NodeDevice) SgmGetNodeContainers() []*ContainerUsage {
	var containerNum C.uint
	C.sgm_get_node_containers(n.node, &containerNum)
	containers := make([]C.sgm_container_t, containerNum)

	containerInfo := make([]*ContainerUsage, int(containerNum))

	var containerUsage C.sgm_node_process_usage_t
	var containerProcessPid *C.uint
	containeridlen := C.uint(12)
	containerProcessNum := C.uint(0)
	var containerId string
	for i := 0; i < int(containerNum); i ++ {

		C.sgm_get_node_container_by_index(n.node, C.uint(i), &containers[i]);
		C.sgm_get_node_container_id(containers[i], containeridlen, C.CString(containerId))
		C.sgm_get_node_container_processes(containers[i], &containerProcessNum, &containerProcessPid)
		C.sgm_get_node_container_usage(n.node, containers[i], &containerUsage, 200)
		processes := make([]uint32, containerProcessNum)
		var pid =  (*[1024]C.uint)(unsafe.Pointer(containerProcessPid))
		for j := 0; j < int(containerProcessNum); j++ {
			processes[j] = uint32(pid[j])
		}
		id, _ := strconv.Atoi(containerId)
		containerInfo[i] = & ContainerUsage{
			Cid:              uint(id),
			RenderUsage:      float32(containerUsage.render_usage),
			BlitterUsage:     float32(containerUsage.blitter_usage),
			VideoUsage:       float32(containerUsage.video_usage),
			VeUsage:          float32(containerUsage.ve_usage),
			MemUsageCreated:  float32(containerUsage.mem_usage_created),
			MemUsageImported: float32(containerUsage.mem_usage_imported),
			ProcessID:        processes,
		}
	}
	//C.free(unsafe.Pointer(containerProcessPid))
	//for i := 0; i < int(containerNum); i ++ {
	//	C.free(unsafe.Pointer(containers[i]))
	//}
	return containerInfo

}




func main()  {
	Init()
	log.Println("C.sgm_init()",C.sgm_init())
	log.Println(" C.geteuid()", C.geteuid())
    cardNum := SgmQueryCardsCount()
	log.Println("count",cardNum)
	//cards, cardInfo := SgmQueryCardInfo(cardNum)
	for i := 0; i < cardNum; i++ {
		dev := SgmQueryCardByIndex(uint(i))
		info := dev.SgmQueryCardInfoByIndex()
		log.Println(info)
		nodeNum := dev.SgmQueryCardNodes()
		if nodeNum == 0 {
			log.Println("We can get no more info for this card.")
			return
		}

		for j := 0; j < nodeNum; j++ {
			node := dev.SgmQueryCardNodeByIndex(uint(j))
			nodeInfo, nodeUsage:= node.SgmQueryNodeInfo(info.Type)
			if nodeInfo == nil && nodeUsage == nil{
				return
			}
			log.Println(nodeInfo, nodeUsage)
			//procInfo := node.SgmGetNodeProcesses()
			//log.Println(procInfo)
			//for i := 0; i < len(procInfo); i ++{
			//	log.Println(procInfo[i].ProcessName)
			//	log.Println(procInfo[i].VeUsage)
			//	log.Println(procInfo[i].MemUsageImported)
			//	log.Println(procInfo[i].MemUsageCreated)
			//	log.Println(procInfo[i].VideoUsage)
			//	log.Println(procInfo[i].BlitterUsage)
			//	log.Println(procInfo[i].Pid)
			//	log.Println(procInfo[i].BlitterFreq)
			//	log.Println(procInfo[i].RenderUsage)
			//
			//}
			container := node.SgmGetNodeContainers()
			for i := 0; i < len(container); i++ {
				log.Println("RenderUsage:", container[i].RenderUsage)
				log.Println("BlitterFreq:", container[i].BlitterFreq)
				log.Println("BlitterUsage:", container[i].BlitterUsage)
				log.Println("VideoUsage:", container[i].VideoUsage)
				log.Println("MemUsageCreated:", container[i].MemUsageCreated)
				log.Println("MemUsageImported:", container[i].MemUsageImported)
				log.Println("VeUsage:", container[i].VeUsage)
				log.Println("ProcessID:", container[i].ProcessID)
				log.Println("Cid:", container[i].Cid)



			}
			log.Println(container)
		}
		dev.DeInit()

	}

	//for i := 0; i < cardNum; i ++ {
	//	processCard(i, cards[i], cardInfo[i])
   //}




}
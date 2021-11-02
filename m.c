/*
// Copyright 2018-2021 Intel Corporation.
//
// This software and the related documents are Intel copyrighted materials,
// and your use of them is governed by the express license under which they
// were provided to you (End User License Agreement for the Intel(R) Software
// Development Products (Version September 2018)). Unless the License provides
// otherwise, you may not use, modify, copy, publish, distribute, disclose or
// transmit this software or the related documents without Intel's prior
// written permission.
//
// This software and the related documents are provided as is, with no
// express or implied warranties, other than those that are expressly
// stated in the License.
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include "sg_monitor.h"

#define PRINT_DELIMITER2()       printf("-----------------------\n")
#define PRINT_DELIMITER()       printf("------------------------------------------------------------------------------------------------------------------------\n")

#define ERR(format,...)     printf("[ERROR] " format,## __VA_ARGS__);

#define IS_VCXA(t)      (ICRM_CARD_TYPE_VCA2 == t || ICRM_CARD_TYPE_VCAC_R == t)
#define IS_VGA(t)       (ICRM_CARD_TYPE_VGA == t)

#define ENABLE_COLOR 1
#if ENABLE_COLOR
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define RESET   "\033[0m"
#else
#define BOLDWHITE   ""      /* No Effect */
#define BOLDBLACK   ""      /* No Effect */
#define BOLDGREEN   ""      /* No Effect */
#define RESET   ""
#endif

static void query_vcxa_node(sgm_card_t card, sgm_node_t node, sgm_node_info_t node_info) {}

static void query_vga_node(sgm_card_t card, sgm_node_t node, sgm_node_info_t node_info) {
    sgm_node_usage_t usage;
    unsigned int timeout_ms = 100;

    if(!strncmp(node_info.status,"READY",5))
    {
        printf("\t\t\t|\tGEN          \t\t%8d    "  "\t|\tGPU Slice Count   \t%8d  \t|\n",node_info.gpu_gen,node_info.gpu_slice_count);
        printf("\t\t\t|\tEU Number         \t%8d \t|\tGPU Subslice Count   \t%8d  \t|\n",node_info.gpu_eu_count,node_info.gpu_subslice_count);
        printf("\t\t\t|\tGPU Base Freq\t\t%8d MHz"   "\t|\tGPU Turbo Freq    \t%8d MHz \t|\n",node_info.gpu_base_freq,node_info.gpu_turbo_freq);
        printf("\t\t\t|\tGPU VDBox Count\t\t%8d  "  "\t|\tGPU L3 Size       \t%8d MB\t|\n",node_info.gpu_vdbox_count,node_info.gpu_l3_size);
         printf("\t\t\t|\tGPU MEM Total (local)\t%8d MB\t|\tPCI Slot ID     \t%8s \t|\n",node_info.gpu_vram_size,node_info.pci_addr);
    }else
        ERR("Fail to get gpu info\n");

    if(sgm_query_node_usage(node, &usage, timeout_ms) == SGM_SUCESS){
        printf("\t\t\t|\tGPU DEV node\t\t%8.7s%3d\t|\t\t\t\t\t\t|\n","renderD",node_info.gpu_index+128);
        printf("\t%10.13s\t|\n","       usage");
        printf("\t\t\t|\tRender Usage \t\t%8.2f %%""\t|\tBlitter Usage        \t%8.2f %%\t|\n",usage.render_usage,usage.blitter_usage);
        printf("\t\t\t|\tVideo Usage   \t\t%8.2f %%" "\t|\tVE Usage             \t%8.2f %%\t|\n",usage.video_usage,usage.ve_usage);      
        printf("\t\t\t|\tGPU MEM Usage (sys)\t%8.2f MB\t|\tGPU MEM Usage (local)\t%8.2f MB\t|\n",usage.mem_usage_gpu_sys,usage.mem_usage_gpu_vram);
        printf("\t\t\t|\tCurrent Power\t\t%8.2f W"  "\t|\tTemperature       \t%8.2f deg.c\t|\n",usage.power,usage.temperature);
        printf("\t\t\t|\tGPU Curr Freq \t\t%8.2f MHz""\t|\t\t\t\t\t\t|\n",usage.render_freq);
    }else
        ERR("Fail to get node usage\n");
}

static void process_card(int card_index, sgm_card_t card, sgm_card_info_t card_info) {
    sgm_node_t *nodes;
    sgm_node_event_t *sgmNodeEvent;
    sgm_node_info_t node_info;
    sgm_container_t *containers;
    sgm_node_process_usage_t *process_usage;
    sgm_node_process_usage_t container_usage;
    unsigned int num_nodes;
    unsigned int timeout_ms = 200;
    unsigned int event_count;
    unsigned int event_index;
    unsigned int process_num;
    unsigned int container_num;
    unsigned int *pids;
    unsigned int pid_name_len = 12;
    char pid_name[pid_name_len];
    unsigned int containerid_len = 12;
    char container_id[containerid_len];
    unsigned int *container_process_pid;
    unsigned int container_process_num;

    sgm_query_card_nodes(card, &num_nodes);
    if (num_nodes == 0)
    {
        printf("\tWe can get no more info for this card.\n");
        return;
    }
    nodes = (sgm_node_t *)calloc(num_nodes, sizeof(sgm_node_t));
    printf("\n");
    PRINT_DELIMITER();
    printf("<CARD>\t" BOLDGREEN "%2d." RESET
           "\ttype: %s\tstatus: %s\tnode_num: %2d\n",
           card_index, card_info.type, card_info.status, num_nodes);
    PRINT_DELIMITER();
    int j;
    int i;
    for (j = 0; j < num_nodes; j++)
    {
        process_num = 0;
        container_num = 0;
        event_count = 0;
        container_process_num = 0;
        sgm_query_card_node_by_index(card, j, &nodes[j]);
        sgm_query_node_info(card, nodes[j], &node_info);
        sgm_set_node_event_threshold(nodes[j], SGM_EVENT_TEMPERATURE_THRESHOLD1, 75); //for configurable event threshold. for testing or other needs. /temp / power / memory
        sgm_set_node_event_threshold(nodes[j], SGM_EVENT_TEMPERATURE_THRESHOLD2, 95);
        sgm_set_node_event_threshold(nodes[j], SGM_EVENT_TEMPERATURE_THRESHOLD3, 100);
        sgm_set_node_event_threshold(nodes[j], SGM_EVENT_POWER_THRESHOLD1, 19);
        sgm_set_node_event_threshold(nodes[j], SGM_EVENT_POWER_THRESHOLD2, 23);
        sgm_set_node_event_threshold(nodes[j], SGM_EVENT_MEMORY_THRESHOLD1, 6900);
        sgm_set_node_event_threshold(nodes[j], SGM_EVENT_MEMORY_THRESHOLD2, 7700);
        sgm_register_node_events(nodes[j], 0xFFF);
        //usleep(500000); //for testing, each thread queries and stores data every 200ms. The longer the waiting time, the more frequently queried events will be found.
        sgm_listen_node_events(nodes[j], &event_count, &sgmNodeEvent, timeout_ms);
        printf("\t\t\t|\n");
        printf("\t<NODE" BOLDGREEN "%d" RESET "> Info\t|-----------------------------------------------------------------------------------------------|\n", j);
        printf("\t\t\t|\tType           \t%18s\t|\tStatus              \t%12s\t|\n", node_info.type, node_info.status);
        if (!strncmp(card_info.type, "SG1", 3) || !strncmp(card_info.type, "V_SG1", 5))
        {
            query_vga_node(card, nodes[j], node_info);
        }
        else if (!strncmp(card_info.type, "VCA2", 4))
        {
            query_vcxa_node(card, nodes[j], node_info);
        }
        else
        {
            free(nodes);
            ERR("Unknown card type %s", card_info.type);
            return;
        }
        if (event_count > 0)
        {

            for (i = 0; i < event_count; i++)
            {
                event_index = sgmNodeEvent[i].occur_num > EVENT_RECORD_NUM ? (EVENT_RECORD_NUM) : (sgmNodeEvent[i].occur_num);
                for (j = 0; j < event_index; j++)
                {
                    if (i == 0 && j == 0)
                    {
                        printf("\n\t\t\t%8s\t%8s\t\t%8s\t%8s\t%8s", "event_flag", "Timestamp(us)", "event_value", "occur_num", "lost_num");
                        printf("\n\t\t\t0x%x\t\t%ld\t%.2f", sgmNodeEvent[i].event_flag, sgmNodeEvent[i].sgm_event_t[j].time_stamp, sgmNodeEvent[i].sgm_event_t[j].event_num);
                        printf("\t\t%lld\t\t%ld", sgmNodeEvent[i].occur_num, sgmNodeEvent[i].event_lost);
                        continue;
                    }
                    if (j == 0)
                    {
                        printf("\n\t\t\t0x%x\t\t%ld\t%.2f", sgmNodeEvent[i].event_flag, sgmNodeEvent[i].sgm_event_t[j].time_stamp, sgmNodeEvent[i].sgm_event_t[j].event_num);
                        printf("\t\t%lld\t\t%ld", sgmNodeEvent[i].occur_num, sgmNodeEvent[i].event_lost);
                        continue;
                    }
                    printf("\n\t\t\t0x%x\t\t%ld\t%.2f", sgmNodeEvent[i].event_flag, sgmNodeEvent[i].sgm_event_t[j].time_stamp, sgmNodeEvent[i].sgm_event_t[j].event_num);
                }
            }
        }
        printf("\n");
        sgm_unregister_node_events(nodes[j], 0x0);
        /* query processes information / usage on node */
        sgm_get_node_processes_num(nodes[j], &process_num);
        sgm_get_node_processes_pids(nodes[j], &pids);
        sgm_get_node_processes_usage(nodes[j], process_num, pids, &process_usage, 200);
        for (i = 0; i < process_num; i++)
        {
            if (i == 0)
            {
                printf("\n\t%s\t\t%5s\t\t%5s\t\t%5s\t\t%5s\t\t%5s\t\t%5s\t%5s", "pid", "process_name", "rcs-%", "blit-%", "vcs-%", "vecs-%", "createdmem-MB", "importedmem-MB");
            }
            sgm_get_node_processes_name(nodes[j], pids[i], pid_name_len, pid_name);
            printf("\n\t%ld\t\t%8s\t\t%.2f\t\t%.2f\t\t%.2f\t\t%.2f\t\t%.2f\t\t%.2f", pids[i], pid_name, process_usage[i].render_usage, process_usage[i].blitter_usage, process_usage[i].video_usage, process_usage[i].ve_usage, process_usage[i].mem_usage_created, process_usage[i].mem_usage_imported);
        }
        /* query containers information / usage on node  */
        sgm_get_node_containers(nodes[j], &container_num);
        containers = (sgm_container_t *)calloc(container_num, sizeof(sgm_container_t));
        for (i = 0; i < container_num; i++)
        {
            sgm_get_node_container_by_index(nodes[j], i, &containers[i]);
            sgm_get_node_container_id(containers[i], containerid_len, container_id);
            sgm_get_node_container_processes(containers[i], &container_process_num, &container_process_pid);
            sgm_get_node_container_usage(nodes[j], containers[i], &container_usage, 200);
            if (i == 0)
                printf("\n\n\t%s\t\t%8s\t%8s\t%8s\t%8s\t%8s\t%8s", "container_id", "rcs-%", "blit-%", "vcs-%", "vecs-%", "createdmem-MB", "importedmem-MB");
            printf("\n\t%8s\t\t%.2f\t\t%.2f\t\t%.2f\t\t%.2f\t\t%.2f\t\t%.2f", container_id, container_usage.render_usage, container_usage.blitter_usage, container_usage.video_usage, container_usage.ve_usage, container_usage.mem_usage_created, container_usage.mem_usage_imported);
            for (j = 0; j < container_process_num; j++)
            {
                if (j == 0)
                {
                    printf("\n\t%8s", "process_pid");
                    printf("\t\t%ld\n", container_process_pid[j]);
                }
                else
                    printf("\t\t\t\t%ld\n", container_process_pid[j]);
            }
            container_process_pid = NULL;
        }
        free(containers);
        containers = NULL;
        pids = NULL;
        process_usage = NULL;
        printf("\n");
    }
    free(nodes);
    nodes = NULL;
}

int display(){
    sgm_system_info_t system_info;
    sgm_card_t *cards;
    sgm_card_info_t *card_info;
    bool onetime;
    unsigned int num_cards;
    onetime = false;
    int i;
    int j;
//    if (argc >= 2)
//    {
//        printf("Please check the input parameters\n");
//        exit(-1);
//    }else
//    {
//        onetime = true;
//    }
    do
    {
        if (sgm_init() == SGM_SUCESS)
        {
            /* TODO: add version info as part of title */
            printf("\n");
            printf("\t\t\t\t\t\t"
                   "====================================\n");
            printf("\t\t\t\t\t\t" BOLDGREEN "* Intel Rendering Resource Monitor *\n" RESET);
            printf("\t\t\t\t\t\t"
                   "====================================\n");
            printf("\n");
            sgm_query_cards(&num_cards);
            cards = (sgm_card_t *)calloc(num_cards, sizeof(sgm_card_t));
            for (i = 0; i < num_cards; i++)
            {
                sgm_query_card_by_index(i, &cards[i]);
            }
            PRINT_DELIMITER2();
            printf("* Number of Cards: " BOLDGREEN "%3d\n" RESET, num_cards);
            PRINT_DELIMITER2();
            card_info = (sgm_card_info_t *)calloc(num_cards, sizeof(sgm_card_info_t));
            for (i = 0; i < num_cards; i++)
            {
                sgm_query_card_info(cards[i], &card_info[i]);
            }
            for (i = 0; i < num_cards; i++)
                process_card(i, cards[i], card_info[i]);
            sgm_deinit(cards);
            free(cards);
            free(card_info);
            cards = NULL;
            card_info = NULL;
            /*
        * deinit will free all the resources including 
        * terminate the local instance query threads
        */
        }
        else if (sgm_init() == SGM_ERROR_INVALID_OPERATION)
        {
            printf("Not Run as root, euid:%ld, please run as root\n", (long)geteuid());
        }
        else
        {
            printf("can't find gpu node, Please confirm whether the card is normal\n");
        }
        usleep(10000);
    } while (!onetime);
}

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
#ifndef __SG_MONITOR_H__
#define __SG_MONITOR_H__
#define EVENT_RECORD_NUM        5

typedef void *sgm_card_t;
typedef void *sgm_node_t;
typedef void *sgm_container_t;

typedef enum _sgm_return {
    SGM_SUCESS                      =   0,
    SGM_ERROR_NOT_READY             =   1,      // not initialized, or needed module or feature not ready
    SGM_ERROR_INVALID_PARAMETER     =   2,      // invalid parameters
    SGM_ERROR_INVALID_OPERATION     =   3,      // invalid, not supported, or no permission operations
    SGM_ERROR_OUT_OF_RESOURCE       =   4,      // out of memory, or out of other sw/hw resources

    SGM_ERROR_UNKNOWN               =   31
}sgm_return_t;

typedef struct _sgm_system_info {
    float        sgm_version;
    unsigned int driver_version;
    unsigned int reserved[6];
}sgm_system_info_t;

typedef struct _sgm_card_info {
    char     type[10];                           // sgm_card_type_t
    char     status[10];                         // sgm_card_status_t
    char     card_reserved[6];
}sgm_card_info_t;

typedef struct _sgm_node_info {
    char type[10];                              // sgm_node_type_t
    char status[10];                            // sgm_node_status_t
    char pci_addr[16];                          // PCI address domain:bus:slot.function

    unsigned int gpu_index;                     // General GPU info
    unsigned int gpu_gen;
    unsigned int gpu_eu_count;
    unsigned int gpu_base_freq;                 //  in MHz
    unsigned int gpu_turbo_freq;                //  in MHz
    unsigned int gpu_vram_size;                 //  in MB

    unsigned int gpu_slice_count;               // Detail GPU info
    unsigned int gpu_subslice_count;
    unsigned int gpu_vdbox_count;
    unsigned int gpu_l3_size;                   // in MB
    unsigned int node_reserved[9];
} sgm_node_info_t;

typedef struct _sgm_node_usage {
    float render_freq;                          // in MHz
    float render_usage;                         // in percentage
    float video_freq;                           // in MHz
    float video_usage;                          // in percentage
    float ve_usage;                             // in percentage
    float blitter_freq;                         // in MHz
    float blitter_usage;                        // in percentage
    float mem_created;                          // GPU MEM usage
    float mem_imported;
    float mem_usage_gpu_sys;                    // in MB
    float mem_usage_gpu_vram;                   // in MB
    float power;                                // in W
    float temperature;                          // in degree
    float reserved[12];
}sgm_node_usage_t;

typedef struct _sgm_event{
    unsigned long long time_stamp;
    float event_num;
}sgm_event_t;

typedef struct _sgm_node_event{
    unsigned int event_flag;
    unsigned int register_flag;
    unsigned int occur_num;
    unsigned int event_index;
    unsigned int event_lost;
    struct _sgm_event sgm_event_t[EVENT_RECORD_NUM];
}sgm_node_event_t;

typedef struct _sgm_node_process_usage {
    float render_usage;                                         // in percentage
    float video_usage;                                          // in percentage
    float ve_usage;                                             // in percentage
    float blitter_freq;                                         // in MHz
    float blitter_usage;                    
    float mem_usage_created;                                    // in MB
    float mem_usage_imported;                                   // in MB
    float reserved[2];
}sgm_node_process_usage_t;

typedef enum _sgm_event_type{
    SGM_EVENT_GLOBAL_RESET                  = 0x00000001,
    SGM_EVENT_RENDER_RESET                  = 0x00000002,
    SGM_EVENT_VIDEO_RESET                   = 0x00000004,
    SGM_EVENT_VE_RESET                      = 0x00000008,
    SGM_EVENT_BLITTER_RESET                 = 0x00000010,

    SGM_EVENT_TEMPERATURE_EXCEEDS_THRESHOLD1 = 0x00000020,      //  temperature > 85% of design max
    SGM_EVENT_TEMPERATURE_EXCEEDS_THRESHOLD2 = 0x00000040,      //  temperature > design max (95 degree for SG1)
    SGM_EVENT_TEMPERATURE_EXCEEDS_THRESHOLD3 = 0x00000080,      //  temperature > Tjmax (100 degree for SG1)

    SGM_EVENT_POWER_EXCEEDS_THRESHOLD1      = 0x00000100,        //  power > 85% of design max
    SGM_EVENT_POWER_EXCEEDS_THRESHOLD2      = 0x00000200,        //  power > design max (23W for SG1)

    SGM_EVENT_MEMORY_EXCEEDS_THRESHOLD1     = 0x00000400,       //  mem usage > 85% of design max
    SGM_EVENT_MEMORY_EXCEEDS_THRESHOLD2     = 0x00000800,       //  mem usage > 95% of design max
} sgm_event_type_t;

typedef enum _sgm_event_threshold_type{
    SGM_EVENT_TEMPERATURE_THRESHOLD1        = 0x00000020,
    SGM_EVENT_TEMPERATURE_THRESHOLD2        = 0x00000040,
    SGM_EVENT_TEMPERATURE_THRESHOLD3        = 0x00000080,

    SGM_EVENT_POWER_THRESHOLD1              = 0x00000100,
    SGM_EVENT_POWER_THRESHOLD2              = 0x00000200,

    SGM_EVENT_MEMORY_THRESHOLD1             = 0x00000400,
    SGM_EVENT_MEMORY_THRESHOLD2             = 0x00000800,
} sgm_event_threshold_type_t;

/* Initialize sg_monintor */
sgm_return_t sgm_init();

/* For query system info */
sgm_return_t sgm_query_system_info(sgm_system_info_t *info);

/* For query cards number|info */
sgm_return_t sgm_query_cards(unsigned int *num_cards);
sgm_return_t sgm_query_card_by_index(unsigned int card_index, sgm_card_t *card);
sgm_return_t sgm_query_card_info(sgm_card_t card, sgm_card_info_t *card_info);

/* For query node number|info */
sgm_return_t sgm_query_card_nodes(sgm_card_t card, unsigned int *num_nodes);
sgm_return_t sgm_query_card_node_by_index(sgm_card_t card, unsigned int node_index, sgm_node_t *node);
sgm_return_t sgm_query_node_info(sgm_card_t card, sgm_node_t node, sgm_node_info_t *node_info);

/* For query node gpu usage, suggest to use 200 as period_ms */
sgm_return_t sgm_query_node_usage(sgm_node_t node, sgm_node_usage_t *usage, unsigned int period_ms);

/* For query process/container on node */
sgm_return_t sgm_get_node_processes_num(sgm_node_t node, unsigned int *process_num);
sgm_return_t sgm_get_node_processes_pids(sgm_node_t node, unsigned int **pids);
sgm_return_t sgm_get_node_processes_name(sgm_node_t node, unsigned int pid, unsigned int name_len, char *name);
sgm_return_t sgm_get_node_processes_usage(sgm_node_t node, unsigned int process_num, unsigned int *pids, sgm_node_process_usage_t **usage, unsigned int period_ms);

sgm_return_t sgm_get_node_containers(sgm_node_t node, unsigned int *container_num);
sgm_return_t sgm_get_node_container_by_index(sgm_node_t node, unsigned int container_index, sgm_container_t *container);
sgm_return_t sgm_get_node_container_id(sgm_container_t container, unsigned int id_len, char *id);
sgm_return_t sgm_get_node_container_processes(sgm_container_t container, unsigned int *process_num, unsigned int **pids);
sgm_return_t sgm_get_node_container_usage(sgm_node_t node, sgm_container_t contaienr, sgm_node_process_usage_t *usage, unsigned int period_ms);

/* For event listen on node*/
sgm_return_t sgm_register_node_events(sgm_node_t node, unsigned int event_flags);
sgm_return_t sgm_set_node_event_threshold(sgm_node_t node, unsigned int event_threshold_type, float value);
sgm_return_t sgm_listen_node_events(sgm_node_t node, unsigned int *event_count, sgm_node_event_t **sgmNodeEvent, unsigned int timeout_ms);
sgm_return_t sgm_unregister_node_events(sgm_node_t nodes, unsigned int event_flags);

/* Release sg_monitor */
sgm_return_t sgm_deinit(sgm_card_t *cards);
#endif // !__SG_MONITOR_H__

#include <stdint.h>
#include <memory.h>

#include <rte_log.h>
#include <rte_mbuf.h>
#include <rte_debug.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_sched.h>
#include <rte_cycles.h>
#include <rte_string_fns.h>
#include <rte_cfgfile.h>
#define APP_RX_DESC_DEFAULT 1024
#define APP_TX_DESC_DEFAULT 1024

#define APP_RING_SIZE (8*1024)
#define NB_MBUF   (2*1024*1024)

#define MAX_PKT_RX_BURST 64
#define PKT_ENQUEUE 64
#define PKT_DEQUEUE 32
#define MAX_PKT_TX_BURST 64

#define RX_PTHRESH 8 /**< Default values of RX prefetch threshold reg. */
#define RX_HTHRESH 8 /**< Default values of RX host threshold reg. */
#define RX_WTHRESH 4 /**< Default values of RX write-back threshold reg. */

#define TX_PTHRESH 36 /**< Default values of TX prefetch threshold reg. */
#define TX_HTHRESH 0  /**< Default values of TX host threshold reg. */
#define TX_WTHRESH 0  /**< Default values of TX write-back threshold reg. */

#define MAX_SCHED_PIPE_PROFILES 256
#define MAX_SCHED_SUBPORTS		8
#define MAX_SCHED_PIPES		4096

struct thread_stat
{
	uint64_t nb_rx;
	uint64_t nb_drop;
};


struct thread_conf
{
	uint32_t counter;
	uint32_t n_mbufs;
	struct rte_mbuf **m_table;

	uint16_t rx_port;
	uint16_t tx_port;
	uint16_t rx_queue;
	uint16_t tx_queue;
	struct rte_ring *rx_ring;
	struct rte_ring *tx_ring;
	struct rte_sched_port *sched_port;

#if APP_COLLECT_STAT
	struct thread_stat stat;
#endif
} __rte_cache_aligned;


struct flow_conf
{
	uint32_t rx_core;
	uint32_t wt_core;
	uint32_t tx_core;
	uint16_t rx_port;
	uint16_t tx_port;
	uint16_t rx_queue;
	uint16_t tx_queue;
	struct rte_ring *rx_ring;
	struct rte_ring *tx_ring;
	struct rte_sched_port *sched_port;
	struct rte_mempool *mbuf_pool;

	struct thread_conf rx_thread;
	struct thread_conf wt_thread;
	struct thread_conf tx_thread;
};


struct ring_conf
{
	uint32_t rx_size;
	uint32_t ring_size;
	uint32_t tx_size;
};

struct burst_conf
{
	uint16_t rx_burst;
	uint16_t ring_burst;
	uint16_t qos_dequeue;
	uint16_t tx_burst;
};

struct ring_thresh
{
	uint8_t pthresh; /**< Ring prefetch threshold. */
	uint8_t hthresh; /**< Ring host threshold. */
	uint8_t wthresh; /**< Ring writeback threshold. */
};
/*global val*/
struct flow_conf qos_conf;


struct ring_conf ring_conf = {
	.rx_size = APP_RX_DESC_DEFAULT,
	.ring_size = APP_RING_SIZE,
	.tx_size = APP_TX_DESC_DEFAULT,
};

struct ring_thresh tx_thresh = {
	.pthresh = TX_PTHRESH,
	.hthresh = TX_HTHRESH,
	.wthresh = TX_WTHRESH,
};

struct ring_thresh rx_thresh = {
	.pthresh = RX_PTHRESH,
	.hthresh = RX_HTHRESH,
	.wthresh = RX_WTHRESH,
};

struct rte_sched_port_params port_params = {
	.name = "port_scheduler_0",
	.socket = 0, /* computed */
	.rate = 0, /* computed */
	.mtu = 6 + 6 + 4 + 4 + 2 + 1500,
	.frame_overhead = RTE_SCHED_FRAME_OVERHEAD_DEFAULT,
	.n_subports_per_port = 1,
	.n_pipes_per_subport = MAX_SCHED_PIPES,
};

static struct rte_sched_pipe_params pipe_profiles[MAX_SCHED_PIPE_PROFILES] = {
	{ /* Profile #0 */
		.tb_rate = 305175,
		.tb_size = 1000000,

		.tc_rate = {305175, 305175, 305175, 305175, 305175, 305175,
			305175, 305175, 305175, 305175, 305175, 305175, 305175},
		.tc_period = 40,
#ifdef RTE_SCHED_SUBPORT_TC_OV
		.tc_ov_weight = 1,
#endif

		.wrr_weights = {1, 1, 1, 1},
	},
};

struct rte_sched_subport_params subport_params[MAX_SCHED_SUBPORTS] = {
	{
		.tb_rate = 1250000000,
		.tb_size = 1000000,

		.tc_rate = {1250000000, 1250000000, 1250000000, 1250000000,
			1250000000, 1250000000, 1250000000, 1250000000, 1250000000,
			1250000000, 1250000000, 1250000000, 1250000000},
		.tc_period = 10,
		.n_pipes_per_subport_enabled = 4096,
		.qsize = {64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64},
		.pipe_profiles = pipe_profiles,
		.n_pipe_profiles = sizeof(pipe_profiles) /
			sizeof(struct rte_sched_pipe_params),
		.n_max_pipe_profiles = MAX_SCHED_PIPE_PROFILES,
#ifdef RTE_SCHED_RED
	.red_params = {
		/* Traffic Class 0 Colors Green / Yellow / Red */
		[0][0] = {.min_th = 48, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[0][1] = {.min_th = 40, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[0][2] = {.min_th = 32, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},

		/* Traffic Class 1 - Colors Green / Yellow / Red */
		[1][0] = {.min_th = 48, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[1][1] = {.min_th = 40, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[1][2] = {.min_th = 32, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},

		/* Traffic Class 2 - Colors Green / Yellow / Red */
		[2][0] = {.min_th = 48, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[2][1] = {.min_th = 40, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[2][2] = {.min_th = 32, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},

		/* Traffic Class 3 - Colors Green / Yellow / Red */
		[3][0] = {.min_th = 48, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[3][1] = {.min_th = 40, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[3][2] = {.min_th = 32, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},

		/* Traffic Class 4 - Colors Green / Yellow / Red */
		[4][0] = {.min_th = 48, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[4][1] = {.min_th = 40, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[4][2] = {.min_th = 32, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},

		/* Traffic Class 5 - Colors Green / Yellow / Red */
		[5][0] = {.min_th = 48, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[5][1] = {.min_th = 40, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[5][2] = {.min_th = 32, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},

		/* Traffic Class 6 - Colors Green / Yellow / Red */
		[6][0] = {.min_th = 48, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[6][1] = {.min_th = 40, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[6][2] = {.min_th = 32, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},

		/* Traffic Class 7 - Colors Green / Yellow / Red */
		[7][0] = {.min_th = 48, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[7][1] = {.min_th = 40, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[7][2] = {.min_th = 32, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},

		/* Traffic Class 8 - Colors Green / Yellow / Red */
		[8][0] = {.min_th = 48, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[8][1] = {.min_th = 40, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[8][2] = {.min_th = 32, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},

		/* Traffic Class 9 - Colors Green / Yellow / Red */
		[9][0] = {.min_th = 48, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[9][1] = {.min_th = 40, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[9][2] = {.min_th = 32, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},

		/* Traffic Class 10 - Colors Green / Yellow / Red */
		[10][0] = {.min_th = 48, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[10][1] = {.min_th = 40, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[10][2] = {.min_th = 32, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},

		/* Traffic Class 11 - Colors Green / Yellow / Red */
		[11][0] = {.min_th = 48, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[11][1] = {.min_th = 40, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[11][2] = {.min_th = 32, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},

		/* Traffic Class 12 - Colors Green / Yellow / Red */
		[12][0] = {.min_th = 48, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[12][1] = {.min_th = 40, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[12][2] = {.min_th = 32, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
	},
#endif /* RTE_SCHED_RED */
	},
};

const char *cfg_profile = NULL;
int mp_size = NB_MBUF;



/*load cfg_file*/
int load_cfg_file();

/*use mempool to buffer pakcet
send packet to rx_ring
use qos sched method
send packet to tx_ring
send packet to output_port*/

int qos_init(void);

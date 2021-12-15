#include "method.h"


int cfg_load_port(struct rte_cfgfile *cfg, struct rte_sched_port_params *port);

int cfg_load_pipe(struct rte_cfgfile *cfg, struct rte_sched_pipe_params *pipe);

int cfg_load_subport(struct rte_cfgfile *cfg, struct rte_sched_subport_params *subport);

int load_cfg_file()
{
    if(cfg_profile == NULL)
        return 0;
    struct rte_cfgfile *file = rte_cfgfile_load(cfg_profile,0);
    if(file == NULL)
        /*use log*/
    
    cfg_load_port(file, &port_params);
    cfg_load_subport(file, subprot_params);
    cfg_load_pip(file, pipe_profiles);

    rte_cfgfile_close(file);
    return 0;
}

/*todo*/
int qos_init()
{
    if(load_cfg_file() !== 0)
    /*Log invalid configuration profile\n!*/
}
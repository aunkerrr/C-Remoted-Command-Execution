#include "common.h"

static int get_system_somaxconn(void) {
    int maxconn = SOMAXCONN;
    FILE *fp = fopen("/proc/sys/net/core/somaxconn", "r");

    if(fp != NULL) {
        if(fscanf(fp, "%d", &maxconn) != 1) {
            maxconn = SOMAXCONN;
        }
        fclose(fp);
    }
    return maxconn;
}

int calculate_optimal_backlog(int requested_backlog) {
    int system_max = get_system_somaxconn();

    if(requested_backlog > 0) {
        if (requested_backlog > system_max) {
            fprintf(stderr, "[WARN] Requested backlog (%d) is greater than core somaxconn (%d). "
                "Core will cut it to (%d). ", 
                requested_backlog, 
                system_max, system_max);

                return system_max;       
        }
        return requested_backlog;
    }

    long nproc = sysconf(_SC_NPROCESSORS_ONLN);
    if (nproc < 1) {
        nproc = 1;
    }

    const int FACTOR_PER_CORE = 1024;

    int calculated = (int) (nproc * FACTOR_PER_CORE);
    
    if (calculated < 1024) {
        calculated = 1024;
    }

    if (calculated > system_max) {
        calculated = system_max;
    }

    return calculated;
}
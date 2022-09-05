#ifndef _LOAD_HPP
#define _LOAD_HPP

# include "util.hpp"

class sched_avg {
    public:
        /* load_avg = runnable % * weight
         * For cfs_rq, load_sum = (runnable time length among LOAD_AVG_MAX) * weight
         * For se, load_sum = (runnable time length among LOAD_AVG_MAX)
         * 
         * util_avg = running % * 1024
         * util_sum = (running time length among LOAD_AVG_MAX) * 1024
         */
        unsigned long   last_update_time;
        unsigned long   load_sum;
        unsigned long   runnable_load_sum;
        unsigned long   util_sum;
        unsigned long   period_contrib;
        unsigned long   load_avg;
        unsigned long   runnable_load_avg;
        unsigned long   util_avg;

        sched_avg() {
            last_update_time = 0;
            load_sum = runnable_load_sum = util_sum = 0;
            period_contrib = 0;
            load_avg = runnable_load_avg = util_avg = 0;
        }

        int accumulate_sum(unsigned long delta, int cpu, unsigned long load, 
                           unsigned long runnable, unsigned long running) {
            unsigned long scale_freq, scale_cpu;
            unsigned long contrib = delta;
            unsigned long periods;

            delta += period_contrib;
            periods = delta / 1024;

            std::cout << "period: " << periods << std::endl;
            std::cout << "load:   " << runnable << std::endl;
            std::cout << "load_sum: " << load_sum << std::endl;

            if (periods) {
                load_sum = decay_load(load_sum, periods);                   // load_sum = old_load_sum * y^period
                runnable_load_sum =
                    decay_load(runnable_load_sum, periods);
                util_sum = decay_load(util_sum, periods);

                delta %= 1024;
                contrib = accumulate_pelt_segments(periods,
                        1024 - period_contrib, delta);
            }

            period_contrib = delta;

            // contrib = contrib / 1024;                                       // Change from us to ms
            if (load)
                load_sum += load * contrib;
            if (runnable)
                runnable_load_sum += runnable * contrib;
            if (running)
                util_sum += contrib * SCHED_CAPACITY_SCALE;

            std::cout << "load_sum: " << load_sum << std::endl;
            std::cout << "contrib:  " << contrib << std::endl;

            return periods;
        }

        int _update_load_sum(unsigned long now, int cpu, int load, int runnable, int running) {
            unsigned long delta;
            
            delta = now - last_update_time;
            delta >>= 10;
            if (!delta)
                return 0;

            last_update_time += delta << 10;

            if (!load) runnable = running = 0;

            if (!accumulate_sum(delta, cpu, load, runnable, running))
                return 0;
            return 1;

        }

        void _update_load_avg(unsigned long load, unsigned long runnable)
        {
            /* total_time   = LOAD_AVG_MAX*y + sa->period_contrib 
             * LOAD_AVG_MAX = 1024(1 + y + y^2 + ... + y^n)
             */
            unsigned long divider = LOAD_AVG_MAX - 1024 + period_contrib;
            std::cout << "previous load avg: " << load_avg << std::endl;

            load_avg            = load * load_sum / divider;                // runnable% * load
            runnable_load_avg   = runnable * runnable_load_sum / divider;
            util_avg            = util_sum / divider;

            std::cout << "new load avg:      " << load_avg << std::endl;
        }

        void debug_load_avg() {
            std::cout << "start printing the info of sched_avg: " << std::endl;
            std::cout << "last_update_time:     " << last_update_time << std::endl;
            std::cout << "load_sum:             " << load_sum << std::endl;
            std::cout << "runnable_load_sum:    " << runnable_load_sum << std::endl;
            std::cout << "util_sum:             " << util_sum << std::endl;
            std::cout << "period_contrib:       " << period_contrib << std::endl;
            std::cout << "load_avg:             " << load_avg << std::endl;
            std::cout << "runnable_load_avg:    " << runnable_load_avg << std::endl;
            std::cout << "util_avg:             " << util_avg << std::endl;
        }
    
    private:
        const unsigned long runnable_avg_yN_inv[32] = {
            0xffffffff, 0xfa83b2da, 0xf5257d14, 0xefe4b99a, 0xeac0c6e6, 0xe5b906e6,
            0xe0ccdeeb, 0xdbfbb796, 0xd744fcc9, 0xd2a81d91, 0xce248c14, 0xc9b9bd85,
            0xc5672a10, 0xc12c4cc9, 0xbd08a39e, 0xb8fbaf46, 0xb504f333, 0xb123f581,
            0xad583ee9, 0xa9a15ab4, 0xa5fed6a9, 0xa2704302, 0x9ef5325f, 0x9b8d39b9,
            0x9837f050, 0x94f4efa8, 0x91c3d373, 0x8ea4398a, 0x8b95c1e3, 0x88980e80,
            0x85aac367, 0x82cd8698
        };
        unsigned long decay_load(unsigned long val, unsigned long n) {
            unsigned int local_n;

            /* after decay for 63 * 32 periods, ignore the old load and just return 0 */ 
            if ((n > LOAD_AVG_PERIOD * 63))
                return 0;

            local_n = n;

            if (local_n >= LOAD_AVG_PERIOD) {
                val >>= local_n / LOAD_AVG_PERIOD;
                local_n %= LOAD_AVG_PERIOD;
            }

            val = (val * runnable_avg_yN_inv[local_n]) >> 32;
            return val;
        }
        unsigned long accumulate_pelt_segments(unsigned long periods, unsigned long d1, unsigned long d3)
        {
            unsigned long c1, c2, c3 = d3; /* y^0 == 1 */

            /*
            * c1 = d1 y^p
            */
            c1 = decay_load(d1, periods);

            /*
            *            p-1
            * c2 = 1024 \Sum y^n
            *            n=1
            *
            *              inf        inf
            *    = 1024 ( \Sum y^n - \Sum y^n - y^0 )
            *              n=0        n=p
            */
            c2 = LOAD_AVG_MAX - decay_load(LOAD_AVG_MAX, periods) - 1024;

            return c1 + c2 + c3;
        }
};

#endif
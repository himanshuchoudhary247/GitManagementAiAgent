#ifndef NPU_H
#define NPU_H

#include <systemc.h>
#include "sfr.h"

/**
 * NPU module that instantiates five SFR modules:
 *   ctrl, status, ifm_info, wt_info, and ofm_info.
 *
 * Provides external methods to write to and read from these SFRs.
 *
 * Lab 3.1 Updates:
 *   - Declares an sc_event m_event_queue.
 *   - Declares an sc_event m_start_schedule.
 *   - Declares two threads:
 *         event_queue_demo_thread: waits on m_event_queue and prints notifications.
 *         schedule_events_thread: waits for m_start_schedule, then schedules three notifications 
 *                                  on m_event_queue (after 2 ns, 1 ns, and 2 ns delays).
 *   - In end_of_elaboration, m_start_schedule is notified.
 */
SC_MODULE(NPU) {
public:
    SC_HAS_PROCESS(NPU);

    NPU(sc_module_name name);
    ~NPU();

    void write(uint32_t offset, const uint32_t &value);
    void read(uint32_t offset, uint32_t &value) const;

    // Event objects
    sc_event m_event_queue;
    sc_event m_start_schedule;

    // Thread processes
    void event_queue_demo_thread();
    void schedule_events_thread();

    // end_of_elaboration callback
    void end_of_elaboration();

private:
    SFR *ctrl;
    SFR *status;
    SFR *ifm_info;
    SFR *wt_info;
    SFR *ofm_info;
};

#endif // NPU_H

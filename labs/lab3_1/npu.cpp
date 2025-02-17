#include "npu.h"
#include <systemc.h>
#include <iostream>

// Offsets for the five SFRs
static const uint32_t CTRL_OFFSET     = 0x00000000;
static const uint32_t STATUS_OFFSET   = 0x00000004;
static const uint32_t IFM_INFO_OFFSET = 0x00000010;
static const uint32_t WT_INFO_OFFSET  = 0x00000014;
static const uint32_t OFM_INFO_OFFSET = 0x00000018;

NPU::NPU(sc_module_name name)
    : sc_module(name)
{
    ctrl = new SFR("ctrl",
                   CTRL_OFFSET, 0xFFFFFFFE, 0x00000001,
                   0xFFFFFFFF, 0x00000000, 0x0);
    status = new SFR("status",
                     STATUS_OFFSET, 0xFFFFFFFF, 0x00000000,
                     0xFFFFFFFF, 0x00000003, 0x0);
    ifm_info = new SFR("ifm_info",
                       IFM_INFO_OFFSET, 0xFFFFFFFF, 0x0FFF0FFF0,
                       0xFFFFFFFF, 0x00000000, 0x0);
    wt_info  = new SFR("wt_info",
                       WT_INFO_OFFSET, 0xFFFFFFFF, 0x0FFF0FFF0,
                       0xFFFFFFFF, 0x00000000, 0x0);
    ofm_info = new SFR("ofm_info",
                       OFM_INFO_OFFSET, 0xFFFFFFFF, 0x0FFF0FFF0,
                       0xFFFFFFFF, 0x00000000, 0x0);

    // Spawn the thread that waits on m_event_queue.
    SC_THREAD(event_queue_demo_thread);
}

NPU::~NPU() {
    delete ctrl;
    delete status;
    delete ifm_info;
    delete wt_info;
    delete ofm_info;
}

void NPU::write(uint32_t offset, const uint32_t &value) {
    switch (offset) {
        case CTRL_OFFSET:
            ctrl->write(value);
            break;
        case STATUS_OFFSET:
            status->write(value);
            break;
        case IFM_INFO_OFFSET:
            ifm_info->write(value);
            break;
        case WT_INFO_OFFSET:
            wt_info->write(value);
            break;
        case OFM_INFO_OFFSET:
            ofm_info->write(value);
            break;
        default:
            std::cout << "[" << name() << "] write: Unknown offset 0x"
                      << std::hex << offset << std::dec << std::endl;
            break;
    }
}

void NPU::read(uint32_t offset, uint32_t &value) const {
    switch (offset) {
        case CTRL_OFFSET:
            ctrl->read(value);
            break;
        case STATUS_OFFSET:
            status->read(value);
            break;
        case IFM_INFO_OFFSET:
            ifm_info->read(value);
            break;
        case WT_INFO_OFFSET:
            wt_info->read(value);
            break;
        case OFM_INFO_OFFSET:
            ofm_info->read(value);
            break;
        default:
            value = 0;
            std::cout << "[" << name() << "] read: Unknown offset 0x"
                      << std::hex << offset << std::dec << std::endl;
            break;
    }
}

void NPU::event_queue_demo_thread() {
    int count = 0;
    while (count < 3) {
        wait(m_event_queue);
        count++;
        std::cout << "[" << name() << "] event_queue_demo_thread triggered (" 
                  << count << "/3) at time " << sc_time_stamp() << std::endl;
    }
    std::cout << "[" << name() << "] event_queue_demo_thread: All 3 events received." << std::endl;
}

void NPU::schedule_events_thread() {
    // This thread schedules three events on m_event_queue.
    wait(2, SC_NS);
    m_event_queue.notify();
    wait(1, SC_NS);  // Total = 3 ns from scheduling start.
    m_event_queue.notify();
    wait(2, SC_NS);  // Total = 5 ns from scheduling start.
    m_event_queue.notify();
}

void NPU::end_of_elaboration() {
    // Instead of immediate notification, schedule m_start_schedule after 1 ns.
    // This avoids immediate notification during elaboration.
    m_start_schedule.notify(1, SC_NS);
    // Spawn a dedicated thread to schedule events.
    SC_THREAD(schedule_events_thread);
}

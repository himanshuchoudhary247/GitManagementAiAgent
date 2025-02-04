#include "npu.h"
#include <iostream>

// Define the static offset constants.
const uint32_t NPU::CTRL_OFFSET    = 0x00000000;
const uint32_t NPU::STATUS_OFFSET  = 0x00000004;
const uint32_t NPU::IFM_INFO_OFFSET = 0x00000010;
const uint32_t NPU::WT_INFO_OFFSET  = 0x00000014;
const uint32_t NPU::OFM_INFO_OFFSET = 0x00000018;

NPU::NPU(sc_core::sc_module_name name)
    : sc_module(name)
{
    // Dynamically instantiate SFR modules.
    ctrl = new SFR("ctrl", CTRL_OFFSET, 0xFFFFFFFE, 0x00000001, 0xFFFFFFFF, 0x00000000, 0x0);
    status = new SFR("status", STATUS_OFFSET, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000003, 0x0);
    ifm_info = new SFR("ifm_info", IFM_INFO_OFFSET, 0xFFFFFFFF, 0x0FFF0FFF, 0xFFFFFFFF, 0x00000000, 0x0);
    wt_info = new SFR("wt_info", WT_INFO_OFFSET, 0xFFFFFFFF, 0x0FFF0FFF, 0xFFFFFFFF, 0x00000000, 0x0);
    ofm_info = new SFR("ofm_info", OFM_INFO_OFFSET, 0xFFFFFFFF, 0x0FFF0FFF, 0xFFFFFFFF, 0x00000000, 0x0);

    // Build the mapping from offsets to SFR pointers.
    sfr_map[CTRL_OFFSET]   = ctrl;
    sfr_map[STATUS_OFFSET] = status;
    sfr_map[IFM_INFO_OFFSET] = ifm_info;
    sfr_map[WT_INFO_OFFSET]  = wt_info;
    sfr_map[OFM_INFO_OFFSET] = ofm_info;

    // Register the event_queue_demo_thread.
    SC_THREAD(event_queue_demo_thread);
    // Note: Within the thread, we use wait(m_event_queue) to wait for event queue notifications.
}

NPU::~NPU() {
    delete ctrl;
    delete status;
    delete ifm_info;
    delete wt_info;
    delete ofm_info;
}

void NPU::write(uint32_t offset, const uint32_t &value) {
    auto it = sfr_map.find(offset);
    if (it != sfr_map.end()) {
        it->second->write(value);
        // If writing to CTRL (offset 0x0) and the start bit is set, trigger operation.
        if (offset == CTRL_OFFSET) {
            uint32_t ctrl_val = 0;
            it->second->read(ctrl_val);
            if (ctrl_val & 0x1) {
                start_operation();
            }
        }
    }
}

void NPU::read(uint32_t offset, uint32_t &value) const {
    auto it = sfr_map.find(offset);
    if (it != sfr_map.end()) {
        it->second->read(value);
    } else {
        value = 0;
    }
}

void NPU::configure_ifm(uint32_t width, uint32_t height) {
    uint32_t config_value = (height << 16) | (width & 0xFFFF);
    ifm_info->write(config_value);
}

void NPU::configure_ofm(uint32_t width, uint32_t height) {
    uint32_t config_value = (height << 16) | (width & 0xFFFF);
    ofm_info->write(config_value);
}

void NPU::configure_weight(uint32_t width, uint32_t height) {
    uint32_t config_value = (height << 16) | (width & 0xFFFF);
    wt_info->write(config_value);
}

void NPU::start_operation() {
    uint32_t status_val = 0;
    status->get(status_val);
    status_val |= 0x1; // Set done bit.
    status->set(status_val);
    std::cout << "[NPU] Operation started at time " << sc_time_stamp() << std::endl;
}

// ---------------------------------------------------------------------
// Event Queue Demo Thread: Waits on m_event_queue notifications
// and prints a message each time it is triggered.
// It should be triggered three times.
void NPU::event_queue_demo_thread() {
    int trigger_count = 0;
    while (trigger_count < 3) {
        // Wait for an event notification from the event queue.
        wait(m_event_queue.default_event());
        trigger_count++;
        std::cout << "[NPU] event_queue_demo_thread triggered (" 
                  << trigger_count << "/3) at time " << sc_time_stamp() << std::endl;
    }
    // Optionally, continue waiting indefinitely.
    while (true) {
        wait();
    }
}

// ---------------------------------------------------------------------
// End-of-elaboration callback.
// This is called after the elaboration phase. We add three events to the
// event queue, scheduled at 2 ns, 3 ns, and 5 ns.
void NPU::end_of_elaboration() {
    // Call base class end_of_elaboration (if necessary).
    sc_module::end_of_elaboration();
    m_event_queue.notify(2, SC_NS);
    m_event_queue.notify(3, SC_NS);
    m_event_queue.notify(5, SC_NS);
}

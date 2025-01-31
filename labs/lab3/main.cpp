#include <systemc.h>
#include <iostream>

SC_MODULE(SimpleModule) {
    SC_CTOR(SimpleModule) {
        SC_THREAD(run);
    }

    void run() {
        std::cout << "[" << sc_time_stamp() << "] SimpleModule: Simulation starts.\n";
        wait(10, SC_NS);
        std::cout << "[" << sc_time_stamp() << "] SimpleModule: Simulation ends.\n";
        sc_stop();
    }
};

int sc_main(int argc, char* argv[]) {
    SimpleModule sm("SimpleModule");
    sc_start();
    std::cout << "Simulation completed at time: " << sc_time_stamp() << "\n";
    return 0;
}

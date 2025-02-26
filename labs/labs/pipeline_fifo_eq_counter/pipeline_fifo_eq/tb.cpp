#include "tb.h"
#include <iostream>
using namespace std;

void TB::producer() {
  // Initialize input signals.
  sig_in_valid.write(false);
  sig_in_data.write(0);
  wait(10, SC_NS);

  // Example 1: Write first data at t = 10 ns.
  cout << "[TB] Producer: Writing first data (101) at " << sc_time_stamp() << endl;
  sig_in_data.write(101);
  sig_in_valid.write(true);
  wait(1, SC_NS);
  sig_in_valid.write(false);

  wait(30, SC_NS);  // Next data arrives at about t = 40 ns.

  cout << "[TB] Producer: Writing second data (202) at " << sc_time_stamp() << endl;
  sig_in_data.write(202);
  sig_in_valid.write(true);
  wait(1, SC_NS);
  sig_in_valid.write(false);

  // Example 2: Send a burst of data.
  for (int i = 0; i < 3; i++) {
    wait(10, SC_NS);  // One cycle gap between burst data.
    sc_uint<32> data = 300 + i;
    cout << "[TB] Producer: Writing burst data " << data 
         << " at " << sc_time_stamp() << endl;
    sig_in_data.write(data);
    sig_in_valid.write(true);
    wait(1, SC_NS);
    sig_in_valid.write(false);
  }
}

void TB::consumer() {
  // Assume the consumer is normally ready.
  sig_out_ready.write(true);

  while (true) {
    wait(1, SC_NS);
    if (sig_out_valid.read()) {
      sc_uint<32> data = sig_out_data.read();
      cout << "[TB] Consumer: Received data " << data 
           << " at " << sc_time_stamp() << endl;
      
      // For demonstration: if data 202 is received, simulate a stall for 3 cycles.
      if (data == 202) {
        cout << "[TB] Consumer: Stalling for 3 cycles at " << sc_time_stamp() << endl;
        sig_out_ready.write(false);
        wait(30, SC_NS);
        sig_out_ready.write(true);
      }
    }
  }
}

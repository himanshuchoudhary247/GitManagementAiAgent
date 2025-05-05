#include "tb.h"
#include <iostream>
using namespace std;

void TB::producer() {
  sig_in_valid.write(false);
  sig_in_data.write(0);
  wait(10, SC_NS);

  cout << "[TB] Producer: Writing first data (101) at " << sc_time_stamp() << endl;
  sig_in_data.write(101);
  sig_in_valid.write(true);
  wait(1, SC_NS);
  sig_in_valid.write(false);

  wait(30, SC_NS);

  cout << "[TB] Producer: Writing second data (202) at " << sc_time_stamp() << endl;
  sig_in_data.write(202);
  sig_in_valid.write(true);
  wait(1, SC_NS);
  sig_in_valid.write(false);

  for (int i = 0; i < 3; i++) {
    wait(10, SC_NS);
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
  sig_out_ready.write(true);

  while (true) {
    wait(1, SC_NS);
    if (sig_out_valid.read()) {
      sc_uint<32> data = sig_out_data.read();
      cout << "[TB] Consumer: Received data " << data 
           << " at " << sc_time_stamp() << endl;
      
      if (data == 202) {
        cout << "[TB] Consumer: Stalling for 3 cycles at " << sc_time_stamp() << endl;
        sig_out_ready.write(false);
        wait(30, SC_NS);
        sig_out_ready.write(true);
      }
    }
  }
}

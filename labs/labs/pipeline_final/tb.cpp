#include "tb.h"
#include <iostream>
using namespace std;

void TB::producer() {
  // Send several data items at specific simulation times.
  wait(10, SC_NS);
  cout << "[TB] Producer: Sending 101 at " << sc_time_stamp() << endl;
  input_fifo.write(101);

  wait(10, SC_NS);
  cout << "[TB] Producer: Sending 102 at " << sc_time_stamp() << endl;
  input_fifo.write(102);

  wait(10, SC_NS);
  cout << "[TB] Producer: Sending 103 at " << sc_time_stamp() << endl;
  input_fifo.write(103);

  wait(10, SC_NS);
  cout << "[TB] Producer: Sending 104 at " << sc_time_stamp() << endl;
  input_fifo.write(104);

  wait(10, SC_NS);
  cout << "[TB] Producer: Sending 105 at " << sc_time_stamp() << endl;
  input_fifo.write(105);
}

void TB::consumer() {
  // Delay consumer start so that some items are buffered.
  wait(120, SC_NS);
  while (true) {
    sc_uint<32> data = output_fifo.read();  // Blocking read.
    cout << "[TB] Consumer: Received " << data << " at " << sc_time_stamp() << endl;
  }
}

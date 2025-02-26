#include <systemc.h>
#include "tb.h"

int sc_main(int argc, char* argv[]) {
  TB tb("tb");
  sc_start(300, SC_NS);
  return 0;
}

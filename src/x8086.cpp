#include "utils/Logger.h"

#include "system/System.h"

int main(const int argc, const char* argv[]) {
  if (argc < 2) {
    MYLOG("'%s' <bios path> <floopy disk path>...");
    return -1;
  }

  System sys;
  sys.initialize(argv[1]);
  sys.run();
  return 0;
}

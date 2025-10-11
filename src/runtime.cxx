#include "runtime.h"
#include <stdexcept>
#include <sstream>

namespace void_compiler {

void bounds_check(int index, int length) {
  if (index < 0 || index >= length) {
    std::ostringstream oss;
    oss << "Index " << index << " out of bounds for length " << length;
    throw std::out_of_range(oss.str());
  }
}

}  // namespace void_compiler
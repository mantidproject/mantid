#include "MantidNexusCpp/napi.h"
#include <iostream>

namespace NexusCppTest {
void print_data(const std::string &prefix, std::ostream &stream, const void *data, const int type, const int num) {
  stream << prefix << " ";
  for (int i = 0; i < num; i++) {
    switch (type) {
    case NX_CHAR:
      stream << (static_cast<const char *>(data))[i];
      break;

    case NX_INT8:
      stream << (static_cast<const unsigned char *>(data))[i];
      break;

    case NX_INT16:
      stream << (static_cast<const short *>(data))[i];
      break;

    case NX_INT32:
      stream << (static_cast<const int *>(data))[i];
      break;

    case NX_INT64:
      stream << static_cast<const int64_t *>(data)[i];
      break;

    case NX_UINT64:
      stream << static_cast<const uint64_t *>(data)[i];
      break;

    case NX_FLOAT32:
      stream << (static_cast<const float *>(data))[i];
      break;

    case NX_FLOAT64:
      stream << (static_cast<const double *>(data))[i];
      break;

    default:
      stream << "print_data: invalid type";
      break;
    }
  }
  stream << "\n";
}
} // namespace NexusCppTest

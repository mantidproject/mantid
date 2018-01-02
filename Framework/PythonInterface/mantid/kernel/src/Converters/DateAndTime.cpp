#include "MantidPythonInterface/kernel/Converters/DateAndTime.h"
#include "MantidPythonInterface/kernel/Converters/NumpyFunctions.h"
#include <numpy/arrayscalars.h>

using Mantid::Types::Core::DateAndTime;
using Mantid::PythonInterface::Converters::Impl::func_PyArray_Descr;

namespace {
// there is a different EPOCH for DateAndTime vs npy_datetime
const npy_datetime UNIX_EPOCH_NS =
    DateAndTime("1970-01-01T00:00").totalNanoseconds();

/* datetime64[ns] from 64bit integer - numpy's interface requires this to be
 * non-const
 * The parts of the string are
 * M = NPY_DATETIMELTR
 * 8 = 8 bit datasize because npy_datetime is a typedef for int64_t
 * [ns] = units description for nanosecond resolution
 */
// PyArray_Descr *NP_DATETIME64_NS_DESCR = func_PyArray_Descr(
//      "M8[ns]");
}

namespace Mantid {
namespace PythonInterface {
namespace Converters {

npy_datetime to_npy_datetime(const DateAndTime &dateandtime) {
  return static_cast<npy_datetime>(dateandtime.totalNanoseconds()) -
         UNIX_EPOCH_NS;
}

PyObject *to_datetime64(const DateAndTime &dateandtime) {
  npy_datetime abstime = to_npy_datetime(dateandtime);
  PyObject *ret =
      PyArray_Scalar(reinterpret_cast<char *>(&abstime), descr_ns(), nullptr);

  return ret;
}

/* datetime64[ns] from 64bit integer - numpy's interface requires this to be
 * non-const
 * The parts of the string are
 * M = NPY_DATETIMELTR
 * 8 = 8 bit datasize because npy_datetime is a typedef for int64_t
 * [ns] = units description for nanosecond resolution
 */
PyArray_Descr *descr_ns() { return func_PyArray_Descr("M8[ns]"); }

Types::Core::DateAndTime to_dateandtime(PyObject *datetime) {
  if (!PyArray_IsScalar(datetime, Datetime)) {
    throw std::runtime_error("Expected datetime64");
  }

  PyDatetimeScalarObject *npdatetime = (PyDatetimeScalarObject *)datetime;
  npy_datetime value = npdatetime->obval;

  // DateAndTime only understands nanoseconds
  switch (npdatetime->obmeta.base) {
  case NPY_FR_s: // second
    value *= 1000000000;
    break;
  case NPY_FR_ms: // milli-second
    value *= 1000000;
    break;
  case NPY_FR_us: // micro-second
    value *= 1000;
    break;
  case NPY_FR_ns: // nanosecond
    break;
  default:
    throw std::runtime_error("Not implemented time unit");
  } // units
  return DateAndTime(UNIX_EPOCH_NS + value);
}

} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid

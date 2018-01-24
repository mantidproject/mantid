#include "MantidPythonInterface/kernel/Converters/DateAndTime.h"
#include "MantidPythonInterface/kernel/Converters/NumpyFunctions.h"
#include <boost/make_shared.hpp>
#include <boost/python.hpp>
#include <numpy/arrayscalars.h>

using Mantid::Types::Core::DateAndTime;
using Mantid::PythonInterface::Converters::Impl::func_PyArray_Descr;

namespace {
// there is a different EPOCH for DateAndTime vs npy_datetime
const npy_datetime UNIX_EPOCH_NS =
    DateAndTime("1970-01-01T00:00").totalNanoseconds();
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

// internal function that handles raw pointer
boost::shared_ptr<Types::Core::DateAndTime>
to_dateandtime(const PyObject *datetime) {
  if (!PyArray_IsScalar(datetime, Datetime)) {
    throw std::runtime_error("Expected datetime64");
  }

  const PyDatetimeScalarObject *npdatetime =
      reinterpret_cast<const PyDatetimeScalarObject *>(datetime);
  npy_datetime value = npdatetime->obval;

  // DateAndTime only understands nanoseconds
  switch (npdatetime->obmeta.base) {
  case NPY_FR_m: // minutes
    value *= 60000000000;
    break;
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
  return boost::make_shared<DateAndTime>(UNIX_EPOCH_NS + value);
}

boost::shared_ptr<Types::Core::DateAndTime>
to_dateandtime(const boost::python::api::object &value) {
  boost::python::extract<Types::Core::DateAndTime> converter_dt(value);
  if (converter_dt.check()) {
    return boost::make_shared<DateAndTime>(converter_dt());
  }

  boost::python::extract<std::string> converter_str(value);
  if (converter_str.check()) {
    return boost::make_shared<DateAndTime>(converter_str());
  }

  boost::python::extract<double> converter_dbl(value);
  if (converter_dbl.check()) {
    return boost::make_shared<DateAndTime>(converter_dbl());
  }

  boost::python::extract<int64_t> converter_int64(value);
  if (converter_int64.check()) {
    return boost::make_shared<DateAndTime>(converter_int64());
  }

  boost::python::extract<int64_t> converter_int32(value);
  if (converter_int32.check()) {
    return boost::make_shared<DateAndTime>(converter_int32());
  }

  // assume it is a numpy.datetime64
  return to_dateandtime(value.ptr());
}

} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid

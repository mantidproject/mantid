#include "MantidPythonInterface/kernel/Converters/PyObjectToString.h"
#include <boost/python/extract.hpp>
#include <boost/python/str.hpp>

using namespace boost;

namespace Mantid {
namespace PythonInterface {
namespace Converters {

std::string pyObjToStr(const python::object &value) {
  python::extract<std::string> extractor(value);

  std::string valuestr;
  if (extractor.check()) {
    valuestr = extractor();
#if PY_VERSION_HEX < 0x03000000
  } else if (PyUnicode_Check(value.ptr())) {
    valuestr =
        python::extract<std::string>(python::str(value).encode("utf-8"))();
#endif
  } else {
    throw std::invalid_argument("Failed to convert python object a string");
  }
  return valuestr;
}

} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid

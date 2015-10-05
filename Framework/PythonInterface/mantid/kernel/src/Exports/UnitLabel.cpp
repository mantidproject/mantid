#include "MantidKernel/UnitLabel.h"
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/return_value_policy.hpp>

#include <boost/make_shared.hpp>
#include <boost/scoped_array.hpp>

using Mantid::Kernel::UnitLabel;
using namespace boost::python;

namespace {
boost::shared_ptr<UnitLabel>
createLabel(const object &ascii, const object &utf8, const object &latex) {
  PyObject *utf8Ptr = utf8.ptr();
  if (PyUnicode_Check(utf8Ptr)) {
    auto length = PyUnicode_GetSize(utf8Ptr);
    typedef UnitLabel::Utf8String::value_type Utf8Char;
    boost::scoped_array<Utf8Char> buffer(new Utf8Char[length]);
    PyUnicode_AsWideChar((PyUnicodeObject *)utf8Ptr, buffer.get(), length);

    auto *rawBuffer = buffer.get();
    return boost::make_shared<UnitLabel>(
        extract<std::string>(ascii)(),
        UnitLabel::Utf8String(rawBuffer, rawBuffer + length),
        extract<std::string>(latex)());
  } else {
    throw std::invalid_argument(
        "utf8 label is not a unicode string object. "
        "Try prefixing the string with a 'u' character.");
  }
}

/**
 * @param self A reference to the calling object
 * @return A new Python unicode string with the contents of the utf8 label
 */
PyObject *utf8ToUnicode(UnitLabel &self) {
  const auto label = self.utf8();
  return PyUnicode_FromWideChar(label.c_str(), label.size());
}
}

void export_UnitLabel() {
  class_<UnitLabel>("UnitLabel", no_init)
      .def("__init__",
           make_constructor(createLabel, default_call_policies(),
                            (arg("ascii"), arg("utf8"), arg("latex"))),
           "Construct a label from a unicode object")

      .def(init<UnitLabel::AsciiString>(
          (arg("ascii")), "Construct a label from a plain-text string"))

      .def("ascii", &UnitLabel::ascii,
           return_value_policy<copy_const_reference>(),
           "Return the label as a plain-text string")

      .def("utf8", &utf8ToUnicode, "Return the label as a unicode string")

      .def("latex", &UnitLabel::latex,
           return_value_policy<copy_const_reference>(),
           "Return the label as a plain-text string with latex formatting")

      // special functions
      .def("__str__", &UnitLabel::ascii,
           return_value_policy<copy_const_reference>())
      .def("__unicode__", &utf8ToUnicode);
}

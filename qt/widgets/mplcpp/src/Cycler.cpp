// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/MplCpp/Cycler.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"

using Mantid::PythonInterface::GlobalInterpreterLock;
using namespace MantidQt::Widgets::Common;

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

namespace {
Python::Object cyclerModule() {
  return Python::NewRef(PyImport_ImportModule("cycler"));
}

/**
 * Creates an iterable from a plain Cycler object
 * @param rawCycler A Cycler object
 * @return An iterable returned from itertools.cycle
 */
Python::Object cycleIterator(const Python::Object &rawCycler) {
  GlobalInterpreterLock lock;
  auto itertools = Python::NewRef(PyImport_ImportModule("itertools"));
  try {
    return Python::Object(itertools.attr("cycle")(rawCycler));
  } catch (Python::ErrorAlreadySet &) {
    throw std::invalid_argument("itertools.cycle() - Object not iterable");
  }
}
} // namespace

/**
 * Create a wrapper around an existing matplotlib.cycler.Cycler object
 * that produces an iterable
 * @param obj An existing instance of a Cycler object
 */
Cycler::Cycler(Python::Object obj)
    : Python::InstanceHolder(cycleIterator(std::move(obj))) {}

/**
 * Advance the iterator and return the previous item
 * @return The next item in the cycle
 */
Python::Dict Cycler::operator()() const {
  GlobalInterpreterLock lock;
  return Python::Dict(Python::NewRef(PyIter_Next(pyobj().ptr())));
}

/**
 * @param label A string label to assign to the cycler. It forms the key for
 * each item return by the cycler
 * @param iterable A string sequence of values to form a cycle
 * @return A new cycler object wrapping the given iterable sequence
 */
Cycler cycler(const char *label, const char *iterable) {
  GlobalInterpreterLock lock;
  return cyclerModule().attr("cycler")(label, iterable);
}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

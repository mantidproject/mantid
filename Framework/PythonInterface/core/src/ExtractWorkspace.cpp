// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/core/ExtractWorkspace.h"

#include <boost/python/extract.hpp>
#include <boost/weak_ptr.hpp>

namespace Mantid {
namespace PythonInterface {

using namespace API;

using boost::python::extract;

//-----------------------------------------------------------------------------
// Public methods
//-----------------------------------------------------------------------------
/**
 * @param pyvalue Python object from which to extract
 */
ExtractWorkspace::ExtractWorkspace(const boost::python::api::object &pyvalue)
    : m_value() {
  // Test for a weak pointer first
  using Workspace_wptr = boost::weak_ptr<Workspace>;
  extract<Workspace_wptr &> extractWeak(pyvalue);
  if (extractWeak.check()) {
    m_value = extractWeak().lock();
  }
  extract<Workspace_sptr &> extractShared(pyvalue);
  if (extractShared.check()) {
    m_value = extractShared();
  }
}

/**
 * Check whether the extract can pull out the workspace type
 * @return True if it can be converted, false otherwise
 */
bool ExtractWorkspace::check() const { return m_value.get() != nullptr; }

/**
 * @return The extracted shared_ptr or throws std::invalid_argument
 */
const API::Workspace_sptr ExtractWorkspace::operator()() const {
  if (check()) {
    return m_value;
  } else {
    throw std::invalid_argument(
        "Unable to extract boost::shared_ptr<Workspace> from Python object");
  }
}
} // namespace PythonInterface
} // namespace Mantid

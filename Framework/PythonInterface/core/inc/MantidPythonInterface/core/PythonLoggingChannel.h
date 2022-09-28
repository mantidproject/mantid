// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//
// PythonLoggingChannel.h
//
// Channel for logging. Sends messages to Python's standard library logging framework.
// Usage: use it in Mantid.properties or mantid.user.properties in addition to, or
// instead of other channel classes.
//

#pragma once

// local includes
#include "MantidPythonInterface/core/DllConfig.h"
#include "MantidPythonInterface/core/WrapPython.h"

// 3rd-party includes
#include <Poco/ConsoleChannel.h>
#include <boost/python/object.hpp>
#include <memory>

namespace Poco {

class MANTID_PYTHONINTERFACE_CORE_DLL PythonLoggingChannel : public Poco::Channel {
public:
  PythonLoggingChannel();
  ~PythonLoggingChannel() override;
  // Because of boost::python::object
  PythonLoggingChannel(const PythonLoggingChannel &) = delete;
  PythonLoggingChannel &operator=(const PythonLoggingChannel &) = delete;
  // Because of Poco::Channel
  PythonLoggingChannel(PythonLoggingChannel &&) = delete;
  PythonLoggingChannel &operator=(PythonLoggingChannel &&) = delete;

  void log(const Poco::Message &msg) override;

private:
  std::unique_ptr<boost::python::object> m_pyLogger;
};

} // namespace Poco

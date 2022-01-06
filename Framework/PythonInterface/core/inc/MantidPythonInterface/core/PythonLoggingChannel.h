// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//
// PythonLoggingChannel.h
//
// TODO
//

#pragma once

// local includes
#include "MantidPythonInterface/core/DllConfig.h"
#include "MantidPythonInterface/core/WrapPython.h"

// 3rd-party includes
#include <Poco/ConsoleChannel.h>
#include <boost/python/object.hpp>

namespace Poco {

class MANTID_PYTHONINTERFACE_CORE_DLL PythonLoggingChannel : public Poco::Channel {
public:
  PythonLoggingChannel();
  ~PythonLoggingChannel() override = default;

  void log(const Poco::Message &msg) override;

private:
  boost::python::object m_pyLogger;
};

} // namespace Poco

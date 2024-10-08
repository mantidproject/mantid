// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidNexusGeometry/DllConfig.h"
#include <memory>
#include <string>

namespace Mantid {
namespace NexusGeometry {

/**
 * Abstract logger. Avoid hard-coded logging dependencies.
 */
class MANTID_NEXUSGEOMETRY_DLL AbstractLogger {
public:
  virtual void debug(const std::string &message) = 0;
  virtual void warning(const std::string &message) = 0;
  virtual void error(const std::string &message) = 0;
  virtual ~AbstractLogger() = default;
};

template <typename T> class LogAdapter : public AbstractLogger {
private:
  T *m_adaptee;

public:
  LogAdapter(T *adaptee) : m_adaptee(adaptee) {}
  virtual void debug(const std::string &message) override { m_adaptee->debug(message); }
  virtual void warning(const std::string &message) override { m_adaptee->warning(message); }
  virtual void error(const std::string &message) override { m_adaptee->error(message); }
};

/**
 * Creates an Adapter and instantiates and returns one as a unique_ptr
 *
 * Make it easy to wrap existing logging frameworks. Note that ownership of
 * adaptee is NOT transferred to returned Logger.
 */
template <typename T> std::unique_ptr<AbstractLogger> makeLogger(T *adaptee) {
  class Adapter : public AbstractLogger {
  private:
    T *m_adaptee;

  public:
    Adapter(T *adaptee) : m_adaptee(adaptee) {}
    virtual void debug(const std::string &message) override { m_adaptee->debug(message); }
    virtual void warning(const std::string &message) override { m_adaptee->warning(message); }
    virtual void error(const std::string &message) override { m_adaptee->error(message); }
  };
  return std::make_unique<Adapter>(adaptee);
}

} // namespace NexusGeometry
} // namespace Mantid

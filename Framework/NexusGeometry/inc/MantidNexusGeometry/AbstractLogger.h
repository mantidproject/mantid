// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_NEXUSGEOMETRY_ABSTRACTLOGGER_H_
#define MANTID_NEXUSGEOMETRY_ABSTRACTLOGGER_H_

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
  virtual void warning(const std::string &warning) = 0;
  virtual void error(const std::string &error) = 0;
  virtual ~AbstractLogger() {}
};

template <typename T>
class MANTID_NEXUSGEOMETRY_DLL LogAdapter : public AbstractLogger {
private:
  T *m_adaptee;

public:
  LogAdapter(T *adaptee) : m_adaptee(adaptee) {}
  virtual void warning(const std::string &message) override {
    m_adaptee->warning(message);
  }
  virtual void error(const std::string &message) override {
    m_adaptee->error(message);
  }
};

/**
 * Creates an Adapter and instantiates and returns one as a unique_ptr
 *
 * Make it easy to wrap existing logging frameworks. Note that ownership of
 * adaptee is NOT transferred to returned Logger.
 */
template <typename T>
MANTID_NEXUSGEOMETRY_DLL std::unique_ptr<AbstractLogger>
makeLogger(T *adaptee) {

  return std::make_unique<LogAdapter<T>>(adaptee);
}

} // namespace NexusGeometry
} // namespace Mantid

#endif /* MANTID_NEXUSGEOMETRY_ABSTRACTLOGGER_H_ */

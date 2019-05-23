// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDNEXUSGEOMETRY_PARSER_H_
#define MANTIDNEXUSGEOMETRY_PARSER_H_

#include "MantidNexusGeometry/DllConfig.h"
#include <memory>
#include <string>

namespace Mantid {
namespace Geometry {
class Instrument;
}
namespace NexusGeometry {
/** NexusGeometryParser : Responsible for parsing a nexus geometry file and
  creating an in-memory Mantid instrument.
*/

MANTID_NEXUSGEOMETRY_DLL class Logger {
public:
  virtual void warning(const std::string &warning) = 0;
  virtual ~Logger() {}
};

template <typename T> std::unique_ptr<Logger> makeLogger(T *adaptee) {

  struct Adapter : public Logger {
    T *m_adaptee;
    Adapter(T *adaptee) : m_adaptee(adaptee) {}
    virtual void warning(const std::string &message) override {
      m_adaptee->warning(message);
    }
  };
  return std::make_unique<Adapter>(adaptee);
}

namespace NexusGeometryParser {

MANTID_NEXUSGEOMETRY_DLL std::unique_ptr<const Mantid::Geometry::Instrument>
createInstrument(const std::string &fileName, std::unique_ptr<Logger> logger);
MANTID_NEXUSGEOMETRY_DLL std::string
getMangledName(const std::string &fileName, const std::string &instName);
} // namespace NexusGeometryParser
} // namespace NexusGeometry
} // namespace Mantid

#endif // MANTIDNEXUSGEOMETRY_PARSER_H_

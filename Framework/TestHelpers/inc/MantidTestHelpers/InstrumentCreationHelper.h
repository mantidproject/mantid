// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef INSTRUMENTCREATIONHELPER_H_
#define INSTRUMENTCREATIONHELPER_H_

#include "MantidTestHelpers/ComponentCreationHelper.h"

namespace Mantid {
namespace API {
class MatrixWorkspace;
}
} // namespace Mantid

namespace InstrumentCreationHelper {

void addFullInstrumentToWorkspace(Mantid::API::MatrixWorkspace &workspace,
                                  bool includeMonitors, bool startYNegative,
                                  const std::string &instrumentName);
Mantid::Geometry::ObjComponent *
addComponent(Mantid::Geometry::Instrument_sptr &instrument,
             const Mantid::Kernel::V3D &position, const std::string &name);
void addSample(Mantid::Geometry::Instrument_sptr &instrument,
               const Mantid::Kernel::V3D &position, const std::string &name);
void addSource(Mantid::Geometry::Instrument_sptr &instrument,
               const Mantid::Kernel::V3D &position, const std::string &name);
void addMonitor(Mantid::Geometry::Instrument_sptr &instrument,
                const Mantid::Kernel::V3D &position, const int ID,
                const std::string &name);
void addDetector(Mantid::Geometry::Instrument_sptr &instrument,
                 const Mantid::Kernel::V3D &position, const int ID,
                 const std::string &name);
} // namespace InstrumentCreationHelper

#endif /* INSTRUMENTCREATIONHELPER_H_ */

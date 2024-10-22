// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
#include "MantidAPI/BoostOptionalToAlgorithmProperty.h"
#include "MantidGeometry/Instrument_fwd.h"
#include "Reduction/Instrument.h"
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL IInstrumentOptionDefaults {
public:
  virtual ~IInstrumentOptionDefaults() = default;
  virtual Instrument get(Mantid::Geometry::Instrument_const_sptr instrument) = 0;
};

/** @class InstrumentOptionDefaults

    This class gets the defaults for the "Instrument" settings tab in the
    reflectometry GUI
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL InstrumentOptionDefaults : public IInstrumentOptionDefaults {
public:
  Instrument get(Mantid::Geometry::Instrument_const_sptr instrument) override;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt

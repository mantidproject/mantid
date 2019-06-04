// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_INSTRUMENTOPTIONDEFAULTS_H
#define MANTID_ISISREFLECTOMETRY_INSTRUMENTOPTIONDEFAULTS_H
#include "Common/DllConfig.h"
#include "MantidAPI/BoostOptionalToAlgorithmProperty.h"
#include "MantidGeometry/Instrument_fwd.h"
#include "Reduction/Instrument.h"
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL IInstrumentOptionDefaults {
public:
  virtual ~IInstrumentOptionDefaults() = default;
  virtual Instrument
  get(Mantid::Geometry::Instrument_const_sptr instrument) = 0;
};

/** @class InstrumentOptionDefaults

    This class gets the defaults for the "Instrument" settings tab in the
    reflectometry GUI
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL InstrumentOptionDefaults
    : public IInstrumentOptionDefaults {
public:
  Instrument get(Mantid::Geometry::Instrument_const_sptr instrument) override;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_INSTRUMENTOPTIONDEFAULTS_H

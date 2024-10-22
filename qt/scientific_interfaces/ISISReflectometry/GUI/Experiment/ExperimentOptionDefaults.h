// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
#include "MantidGeometry/Instrument_fwd.h"
#include "Reduction/Experiment.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL IExperimentOptionDefaults {
public:
  virtual ~IExperimentOptionDefaults() = default;
  virtual Experiment get(Mantid::Geometry::Instrument_const_sptr instrument) = 0;
};

/** @class ExperimentOptionDefaults

    This class gets the defaults for the "Experiment" settings tab in the
    reflectometry GUI
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL ExperimentOptionDefaults : public IExperimentOptionDefaults {
public:
  Experiment get(Mantid::Geometry::Instrument_const_sptr instrument) override;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt

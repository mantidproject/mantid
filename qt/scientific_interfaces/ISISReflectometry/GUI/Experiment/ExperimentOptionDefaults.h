// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_EXPERIMENTOPTIONDEFAULTS_H
#define MANTID_ISISREFLECTOMETRY_EXPERIMENTOPTIONDEFAULTS_H
#include "Common/DllConfig.h"
#include "MantidAPI/BoostOptionalToAlgorithmProperty.h"
#include "MantidGeometry/Instrument_fwd.h"
#include "Reduction/Experiment.h"
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class IExperimentOptionDefaults {
public:
  virtual Experiment operator()(Mantid::Geometry::Instrument_const_sptr instrument) = 0;
};

class ExperimentOptionDefaults : public IExperimentOptionDefaults {
public:
  Experiment operator()(Mantid::Geometry::Instrument_const_sptr instrument) override;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_EXPERIMENTOPTIONDEFAULTS_H

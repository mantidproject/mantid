// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IEXPERIMENTPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_IEXPERIMENTPRESENTER_H

#include "GUI/Batch/IBatchPresenter.h"
#include "MantidGeometry/Instrument_fwd.h"
#include "Reduction/Experiment.h"
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
class Instrument_const_sptr;

class IExperimentPresenter {
public:
  virtual ~IExperimentPresenter() = default;

  virtual void acceptMainPresenter(IBatchPresenter *mainPresenter) = 0;
  virtual Experiment const &experiment() const = 0;
  virtual void reductionPaused() = 0;
  virtual void reductionResumed() = 0;
  virtual void autoreductionPaused() = 0;
  virtual void autoreductionResumed() = 0;
  virtual void
  instrumentChanged(std::string const &instrumentName,
                    Mantid::Geometry::Instrument_const_sptr instrument) = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IEXPERIMENTPRESENTER_H */

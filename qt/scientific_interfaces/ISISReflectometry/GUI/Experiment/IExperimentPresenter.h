// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "GUI/Batch/IBatchPresenter.h"
#include "MantidGeometry/Instrument_fwd.h"
#include "Reduction/Experiment.h"
#include "Reduction/PreviewRow.h"
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
class Instrument_const_sptr;

class IExperimentPresenter {
public:
  virtual ~IExperimentPresenter() = default;

  virtual void acceptMainPresenter(IBatchPresenter *mainPresenter) = 0;
  virtual Experiment const &experiment() const = 0;
  virtual void notifyReductionPaused() = 0;
  virtual void notifyReductionResumed() = 0;
  virtual void notifyAutoreductionPaused() = 0;
  virtual void notifyAutoreductionResumed() = 0;
  virtual void notifyInstrumentChanged(std::string const &instrumentName) = 0;
  virtual void notifyPreviewApplyRequested(PreviewRow const &previewRow) = 0;
  virtual void restoreDefaults() = 0;
  virtual bool hasValidSettings() const noexcept = 0;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt

// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class IBatchPresenter;

class IPlottingPresenter {
public:
  virtual ~IPlottingPresenter() = default;

  virtual void acceptMainPresenter(IBatchPresenter *mainPresenter) = 0;
  virtual void notifyReductionPaused() = 0;
  virtual void notifyReductionResumed() = 0;
  virtual void notifyAutoreductionPaused() = 0;
  virtual void notifyAutoreductionResumed() = 0;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry

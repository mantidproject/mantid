// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "../../Reduction/Experiment.h"
#include "Common/DllConfig.h"
#include "ExperimentPresenter.h"
#include "IExperimentPresenter.h"
#include "IExperimentView.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class ExperimentPresenterFactory {
public:
  explicit ExperimentPresenterFactory(IFileHandler *fileHandler, double thetaTolerance)
      : m_fileHandler(fileHandler), m_thetaTolerance(thetaTolerance) {}

  std::unique_ptr<IExperimentPresenter> make(IExperimentView *view) {
    return std::make_unique<ExperimentPresenter>(view, Experiment(), m_thetaTolerance, m_fileHandler);
  }

private:
  IFileHandler *m_fileHandler;
  double m_thetaTolerance;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt

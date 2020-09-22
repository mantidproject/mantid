// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "RunsTablePresenterFactory.h"

#include "RunsTablePresenter.h"
#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

RunsTablePresenterFactory::RunsTablePresenterFactory(
    std::vector<std::string> instruments, double thetaTolerance,
    IClipboardFactory *cbf, Plotter plotter)
    : m_instruments(std::move(instruments)), m_thetaTolerance(thetaTolerance), m_cbf(cbf),
      m_plotter(std::move(plotter)) {}

std::unique_ptr<IRunsTablePresenter> RunsTablePresenterFactory::
operator()(IRunsTableView *view) const {
  return std::make_unique<RunsTablePresenter>(
      view, m_instruments, m_thetaTolerance, ReductionJobs(), m_cbf, m_plotter);
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry

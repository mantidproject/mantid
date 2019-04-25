// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "RunsTablePresenterFactory.h"
#include "MantidKernel/make_unique.h"
#include "RunsTablePresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

RunsTablePresenterFactory::RunsTablePresenterFactory(
    std::vector<std::string> const &instruments, double thetaTolerance,
    Plotter plotter)
    : m_instruments(instruments), m_thetaTolerance(thetaTolerance),
      m_plotter(std::move(plotter)) {}

std::unique_ptr<IRunsTablePresenter> RunsTablePresenterFactory::
operator()(IRunsTableView *view) const {
  return Mantid::Kernel::make_unique<RunsTablePresenter>(
      view, m_instruments, m_thetaTolerance, ReductionJobs(), m_plotter);
}
} // namespace CustomInterfaces
} // namespace MantidQt

// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "RunsTable.h"

namespace MantidQt {
namespace CustomInterfaces {

RunsTable::RunsTable(std::vector<std::string> instruments,
                     double thetaTolerance, ReductionJobs reductionJobs)
    : m_instruments(std::move(instruments)), m_thetaTolerance(thetaTolerance),
      m_reductionJobs(std::move(reductionJobs)) {}

double RunsTable::thetaTolerance() const { return m_thetaTolerance; }

ReductionJobs const &RunsTable::reductionJobs() const {
  return m_reductionJobs;
}

ReductionJobs &RunsTable::reductionJobs() { return m_reductionJobs; }

} // namespace CustomInterfaces
} // namespace MantidQt

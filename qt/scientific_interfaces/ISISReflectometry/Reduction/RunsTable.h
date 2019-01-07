// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTID_CUSTOMINTERFACES_RUNSTABLE_H_
#define MANTID_CUSTOMINTERFACES_RUNSTABLE_H_

#include "ReductionJobs.h"
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

class RunsTable {
public:
  RunsTable(std::vector<std::string> instruments, double thetaTolerance,
            ReductionJobs ReductionJobs);

  double thetaTolerance() const;
  ReductionJobs const &reductionJobs() const;
  ReductionJobs &reductionJobs();

private:
  std::vector<std::string> m_instruments;
  double m_thetaTolerance;
  ReductionJobs m_reductionJobs;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_RUNSTABLE_H_

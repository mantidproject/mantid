// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/MultiPeriodGroupWorker.h"
#include <boost/scoped_ptr.hpp>

namespace Mantid {
namespace API {

/** MutliPeriodGroupAlgorithm : Abstract algorithm. Algorithms that need special
processing for Mutli-Period group workspaces should inherit from this
algorithm rather than from Algorithm directly. This algorithm processes
workspaces in each group input in a pair-wise fashion to give a group workspace
output.
  */
class MANTID_API_DLL MultiPeriodGroupAlgorithm : public Algorithm {
public:
  MultiPeriodGroupAlgorithm();

private:
  /// Overriden from Algorithm base
  bool checkGroups() override;
  /// Overriden from Algorithm base.
  bool processGroups() override;
  /// Method to provide the name for the input workspace property.
  virtual std::string fetchInputPropertyName() const = 0;
  /// Method to indicate that a non-standard property is taken as the input, so
  /// will be specified via fetchInputPropertyName.
  virtual bool useCustomInputPropertyName() const { return false; }

  /// Convenience typdef for workspace names.
  using VecWSGroupType = MultiPeriodGroupWorker::VecWSGroupType;
  /// multi period group workspaces.
  VecWSGroupType m_multiPeriodGroups;
  /// Multiperiod group worker.
  boost::scoped_ptr<MultiPeriodGroupWorker> m_worker;
};

} // namespace API
} // namespace Mantid

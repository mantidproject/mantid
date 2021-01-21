// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidSINQ/DllConfig.h"

#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidSINQ/PoldiUtilities/PoldiPeakCollection.h"

namespace Mantid {
namespace Poldi {

/** PoldiPeakSummary

  This small algorithm produces a summary table for a given
  PoldiPeakCollection, similar to what the original data
  analysis software produced.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 03/12/2014
*/
class MANTID_SINQ_DLL PoldiPeakSummary : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

protected:
  DataObjects::TableWorkspace_sptr getSummaryTable(const PoldiPeakCollection_sptr &peakCollection) const;
  DataObjects::TableWorkspace_sptr getInitializedResultWorkspace() const;

  void storePeakSummary(API::TableRow tableRow, const PoldiPeak_sptr &peak) const;

private:
  void init() override;
  void exec() override;
};

} // namespace Poldi
} // namespace Mantid

// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidSINQ/DllConfig.h"
#include "MantidSINQ/PoldiUtilities/PoldiPeakCollection.h"

namespace Mantid {
namespace Poldi {
/** PoldiFitPeaks1D :

  PoldiFitPeaks1D fits multiple peaks to POLDI auto-correlation data.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 17/03/2014
*/
class MANTID_SINQ_DLL PoldiFitPeaks1D : public API::Algorithm {
public:
  virtual ~PoldiFitPeaks1D() = default;
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "PoldiPeakFit1D fits peak profiles to POLDI auto-correlation data.";
  }

  int version() const override;
  const std::string category() const override;

protected:
  void setPeakFunction(const std::string &peakFunction);
  PoldiPeakCollection_sptr getInitializedPeakCollection(const DataObjects::TableWorkspace_sptr &peakTable) const;

  API::IFunction_sptr getPeakProfile(const PoldiPeak_sptr &poldiPeak) const;
  void setValuesFromProfileFunction(const PoldiPeak_sptr &poldiPeak, const API::IFunction_sptr &fittedFunction) const;
  double getFwhmWidthRelation(const API::IPeakFunction_sptr &peakFunction) const;

  API::IAlgorithm_sptr getFitAlgorithm(const DataObjects::Workspace2D_sptr &dataWorkspace, const PoldiPeak_sptr &peak,
                                       const API::IFunction_sptr &profile);

  PoldiPeakCollection_sptr m_peaks;
  std::string m_profileTemplate;
  API::IFunction_sptr m_backgroundTemplate;
  std::string m_profileTies;

  double m_fwhmMultiples{1.0};

private:
  void init() override;
  void exec() override;
};

} // namespace Poldi
} // namespace Mantid

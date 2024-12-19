// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidSINQ/DllConfig.h"

#include "MantidSINQ/PoldiUtilities/PoldiAbstractChopper.h"

namespace Mantid {
namespace Poldi {

/** PoldiTruncateData :

    POLDI data time bin count and width are directly connected to the
    chopper rotation speed. In the raw data, there are some additional
    bins at the end of each spectrum. These extra bins should not contain
    any data. If there are more than just a few extra counts in those bins,
    something is wrong with the measurement.

    This algorithm checks these extra bins (if present) and outputs
    a table with the counts in each bin summed over all spectra.
    Then these extra bins are removed, so data analysis can carry on properly.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 11/06/2014
  */

class DLLExport PoldiTruncateData : public API::Algorithm {
public:
  PoldiTruncateData();
  virtual ~PoldiTruncateData() = default;
  const std::string name() const override { return "PoldiTruncateData"; }
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

  size_t getCalculatedBinCount();
  size_t getActualBinCount();

protected:
  void setChopperFromWorkspace(const API::MatrixWorkspace_const_sptr &workspace);
  void setChopper(PoldiAbstractChopper_sptr chopper);

  void setTimeBinWidthFromWorkspace(const API::MatrixWorkspace_const_sptr &workspace);
  void setTimeBinWidth(double timeBinWidth);
  void setActualBinCount(size_t actualBinCount);

  double getMaximumTimeValue(size_t calculatedBinCount);
  double getMinimumExtraTimeValue(size_t calculatedBinCount);

  API::MatrixWorkspace_sptr getCroppedWorkspace(const API::MatrixWorkspace_sptr &workspace);
  API::MatrixWorkspace_sptr getExtraCountsWorkspace(const API::MatrixWorkspace_sptr &workspace);

  API::MatrixWorkspace_sptr getWorkspaceBelowX(const API::MatrixWorkspace_sptr &workspace, double x);
  API::MatrixWorkspace_sptr getWorkspaceAboveX(const API::MatrixWorkspace_sptr &workspace, double x);

  API::Algorithm_sptr getCropAlgorithmForWorkspace(const API::MatrixWorkspace_sptr &workspace);
  API::MatrixWorkspace_sptr getOutputWorkspace(const API::Algorithm_sptr &algorithm);

  API::MatrixWorkspace_sptr getSummedSpectra(const API::MatrixWorkspace_sptr &workspace);

  PoldiAbstractChopper_sptr m_chopper;
  double m_timeBinWidth;
  size_t m_actualBinCount;

private:
  void init() override;
  void exec() override;
};

} // namespace Poldi
} // namespace Mantid

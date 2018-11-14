// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_SINQ_POLDITRUNCATEDATA_H_
#define MANTID_SINQ_POLDITRUNCATEDATA_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

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
  const std::string name() const override { return "PoldiTruncateData"; }
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

  size_t getCalculatedBinCount();
  size_t getActualBinCount();

protected:
  void setChopperFromWorkspace(API::MatrixWorkspace_const_sptr workspace);
  void setChopper(PoldiAbstractChopper_sptr chopper);

  void setTimeBinWidthFromWorkspace(API::MatrixWorkspace_const_sptr workspace);
  void setTimeBinWidth(double timeBinWidth);
  void setActualBinCount(size_t actualBinCount);

  double getMaximumTimeValue(size_t calculatedBinCount);
  double getMinimumExtraTimeValue(size_t calculatedBinCount);

  API::MatrixWorkspace_sptr
  getCroppedWorkspace(API::MatrixWorkspace_sptr workspace);
  API::MatrixWorkspace_sptr
  getExtraCountsWorkspace(API::MatrixWorkspace_sptr workspace);

  API::MatrixWorkspace_sptr
  getWorkspaceBelowX(API::MatrixWorkspace_sptr workspace, double x);
  API::MatrixWorkspace_sptr
  getWorkspaceAboveX(API::MatrixWorkspace_sptr workspace, double x);

  API::Algorithm_sptr
  getCropAlgorithmForWorkspace(API::MatrixWorkspace_sptr workspace);
  API::MatrixWorkspace_sptr getOutputWorkspace(API::Algorithm_sptr algorithm);

  API::MatrixWorkspace_sptr
  getSummedSpectra(API::MatrixWorkspace_sptr workspace);

  PoldiAbstractChopper_sptr m_chopper;
  double m_timeBinWidth;
  size_t m_actualBinCount;

private:
  void init() override;
  void exec() override;
};

} // namespace Poldi
} // namespace Mantid

#endif /* MANTID_SINQ_POLDITRUNCATEDATA_H_ */

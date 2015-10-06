#ifndef MANTID_SINQ_POLDITRUNCATEDATA_H_
#define MANTID_SINQ_POLDITRUNCATEDATA_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

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

    Copyright © 2014 PSI-MSS

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */

class DLLExport PoldiTruncateData : public API::Algorithm {
public:
  PoldiTruncateData();
  virtual ~PoldiTruncateData() {}

  virtual const std::string name() const { return "PoldiTruncateData"; }
  virtual int version() const;
  virtual const std::string category() const;
  virtual const std::string summary() const;

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
  void init();
  void exec();
};

} // namespace SINQ
} // namespace Mantid

#endif /* MANTID_SINQ_POLDITRUNCATEDATA_H_ */

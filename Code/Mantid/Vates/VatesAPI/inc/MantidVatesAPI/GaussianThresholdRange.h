#ifndef MANTID_PARAVIEW_GAUSSIAN_THRESHOLD_RANGE
#define MANTID_PARAVIEW_GAUSSIAN_THRESHOLD_RANGE

#include "MantidKernel/System.h"
#include "MantidVatesAPI/ThresholdRange.h"
#include "MantidAPI/IMDWorkspace.h"

/** Caclulates range values based on the distribution of signal values in the workspace.

 @author Owen Arnold, Tessella plc
 @date 30/06/2011

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

 File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */

namespace Mantid
{
namespace VATES
{
class DLLExport GaussianThresholdRange : public ThresholdRange
{

public:

  GaussianThresholdRange(Mantid::API::IMDWorkspace_sptr workspace, double preferred_nStd = 1, size_t sampleSize = 100);

  GaussianThresholdRange(double preferred_nStd = 1, size_t sampleSize = 100);

  virtual void setWorkspace(Mantid::API::IMDWorkspace_sptr workspace);

  virtual void calculate();

  virtual bool hasCalculated() const;

  virtual signal_t getMinimum() const;

  virtual signal_t getMaximum() const;

  virtual GaussianThresholdRange* clone() const;

  virtual ~GaussianThresholdRange();

private:
  
  void calculateAsNormalDistrib(std::vector<signal_t>& raw_values, size_t size, signal_t max_signal, signal_t min_signal, signal_t accumulated_signal);

  Mantid::API::IMDWorkspace_sptr m_workspace;

  signal_t m_min;
  
  signal_t m_max;

  bool m_isCalculated;

  signal_t m_preferred_nStd;

  size_t m_sampleSize; 

};
}
}

#endif
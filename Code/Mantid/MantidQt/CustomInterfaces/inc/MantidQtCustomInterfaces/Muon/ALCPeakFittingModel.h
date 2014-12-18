#ifndef MANTID_CUSTOMINTERFACES_ALCPEAKFITTINGMODEL_H_
#define MANTID_CUSTOMINTERFACES_ALCPEAKFITTINGMODEL_H_

#include "MantidKernel/System.h"

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Muon/IALCPeakFittingModel.h"

using namespace Mantid::API;

namespace MantidQt
{
namespace CustomInterfaces
{

  /** ALCPeakFittingModel : Concrete model for ALC peak fitting
    
    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class MANTIDQT_CUSTOMINTERFACES_DLL ALCPeakFittingModel : public IALCPeakFittingModel
  {
  public:
    // -- IALCPeakFittingModel interface -----------------------------------------------------------
    IFunction_const_sptr fittedPeaks() const { return m_fittedPeaks; }
    MatrixWorkspace_const_sptr data() const { return m_data; }

    void fitPeaks(IFunction_const_sptr peaks);
    // -- End of IALCPeakFittingModel interface ----------------------------------------------------

    /// Update the data
    void setData(MatrixWorkspace_const_sptr newData);

    /// Export data and fitted peaks as a single workspace
    MatrixWorkspace_sptr exportWorkspace();

    /// Export fitted peaks as a table workspace
    ITableWorkspace_sptr exportFittedPeaks();

  private:
    /// The data we are fitting peaks to
    MatrixWorkspace_const_sptr m_data;

    /// Setter for convenience
    void setFittedPeaks(IFunction_const_sptr fittedPeaks);

    /// Last fitted peaks
    IFunction_const_sptr m_fittedPeaks;
  };


} // namespace CustomInterfaces
} // namespace MantidQt

#endif  /* MANTID_CUSTOMINTERFACES_ALCPEAKFITTINGMODEL_H_ */

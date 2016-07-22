#ifndef MANTID_CUSTOMINTERFACES_ALCPEAKFITTINGMODEL_H_
#define MANTID_CUSTOMINTERFACES_ALCPEAKFITTINGMODEL_H_

#include "MantidKernel/System.h"

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Muon/IALCPeakFittingModel.h"
#include "MantidAPI/ITableWorkspace_fwd.h"

namespace MantidQt {
namespace CustomInterfaces {

/** ALCPeakFittingModel : Concrete model for ALC peak fitting

  Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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
class MANTIDQT_CUSTOMINTERFACES_DLL ALCPeakFittingModel
    : public IALCPeakFittingModel {
public:
  // -- IALCPeakFittingModel interface
  // -----------------------------------------------------------
  Mantid::API::IFunction_const_sptr fittedPeaks() const override {
    return m_fittedPeaks;
  }
  Mantid::API::MatrixWorkspace_const_sptr data() const override {
    return m_data;
  }
  Mantid::API::ITableWorkspace_sptr parameterTable() const {
    return m_parameterTable;
  }

  void fitPeaks(Mantid::API::IFunction_const_sptr peaks) override;
  // -- End of IALCPeakFittingModel interface
  // ----------------------------------------------------

  /// Update the data
  void setData(Mantid::API::MatrixWorkspace_const_sptr newData);

  /// Export data and fitted peaks as a single workspace
  Mantid::API::MatrixWorkspace_sptr exportWorkspace();

  /// Export fitted peaks as a table workspace
  Mantid::API::ITableWorkspace_sptr exportFittedPeaks();

private:
  /// The data we are fitting peaks to
  Mantid::API::MatrixWorkspace_const_sptr m_data;

  /// Parameter table containing fit results
  Mantid::API::ITableWorkspace_sptr m_parameterTable;

  /// Setter for convenience
  void setFittedPeaks(Mantid::API::IFunction_const_sptr fittedPeaks);

  /// Last fitted peaks
  Mantid::API::IFunction_const_sptr m_fittedPeaks;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_CUSTOMINTERFACES_ALCPEAKFITTINGMODEL_H_ */

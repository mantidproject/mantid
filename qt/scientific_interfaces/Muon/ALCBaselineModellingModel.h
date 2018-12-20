#ifndef MANTID_CUSTOMINTERFACES_ALCBASELINEMODELLINGMODEL_H_
#define MANTID_CUSTOMINTERFACES_ALCBASELINEMODELLINGMODEL_H_

#include "DllConfig.h"
#include "MantidKernel/System.h"

#include "IALCBaselineModellingModel.h"
#include "MantidAPI/ITableWorkspace_fwd.h"

namespace MantidQt {
namespace CustomInterfaces {
/** ALCBaselineModellingModel : Concrete ALC Baseline Modelling step model
  implementation.

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
class MANTIDQT_MUONINTERFACE_DLL ALCBaselineModellingModel
    : public IALCBaselineModellingModel {
public:
  // -- IALCBaselineModellingModel interface
  // -----------------------------------------------------

  Mantid::API::MatrixWorkspace_const_sptr data() const override;

  void fit(Mantid::API::IFunction_const_sptr function,
           const std::vector<Section> &sections) override;

  Mantid::API::IFunction_const_sptr fittedFunction() const override {
    return m_fittedFunction;
  }

  Mantid::API::MatrixWorkspace_const_sptr correctedData() const override;

  Mantid::API::ITableWorkspace_sptr parameterTable() const {
    return m_parameterTable;
  }

  const std::vector<Section> &sections() const { return m_sections; }

  // -- End of IALCBaselineModellingModel interface
  // ----------------------------------------------

  /// Set the data we should fit baseline for
  void setData(Mantid::API::MatrixWorkspace_const_sptr data);

  /// Set the corrected data resulting from fit
  void setCorrectedData(Mantid::API::MatrixWorkspace_const_sptr data);

  /// Export data + baseline + corrected data as a single workspace
  Mantid::API::MatrixWorkspace_sptr exportWorkspace();

  /// Export sections used for the last fit as a table workspace
  Mantid::API::ITableWorkspace_sptr exportSections();

  /// Exports baseline model as a table workspace
  Mantid::API::ITableWorkspace_sptr exportModel();

private:
  /// Data used for fitting
  Mantid::API::MatrixWorkspace_const_sptr m_data;

  /// Result function of the last fit
  Mantid::API::IFunction_const_sptr m_fittedFunction;

  /// Fit table containing parameters and errors
  Mantid::API::ITableWorkspace_sptr m_parameterTable;

  /// Sections used for the last fit
  std::vector<Section> m_sections;

  // Setters for convenience
  void setFittedFunction(Mantid::API::IFunction_const_sptr function);

  // Set errors in the ws after the fit
  void setErrorsAfterFit(Mantid::API::MatrixWorkspace_sptr data);

  /// Disables points which shouldn't be used for fitting
  static void disableUnwantedPoints(Mantid::API::MatrixWorkspace_sptr ws,
                                    const std::vector<Section> &sections);

  /// Enable previously disabled points
  static void
  enableDisabledPoints(Mantid::API::MatrixWorkspace_sptr destWs,
                       Mantid::API::MatrixWorkspace_const_sptr sourceWs);
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_CUSTOMINTERFACES_ALCBASELINEMODELLINGMODEL_H_ */

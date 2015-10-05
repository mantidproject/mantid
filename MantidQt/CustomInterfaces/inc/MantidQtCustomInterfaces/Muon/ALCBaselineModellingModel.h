#ifndef MANTID_CUSTOMINTERFACES_ALCBASELINEMODELLINGMODEL_H_
#define MANTID_CUSTOMINTERFACES_ALCBASELINEMODELLINGMODEL_H_

#include "MantidKernel/System.h"
#include "MantidQtCustomInterfaces/DllConfig.h"

#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidQtCustomInterfaces/Muon/IALCBaselineModellingModel.h"

using namespace Mantid::API;

namespace MantidQt
{
namespace CustomInterfaces
{
  /** ALCBaselineModellingModel : Concrete ALC Baseline Modelling step model implementation.
    
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
  class MANTIDQT_CUSTOMINTERFACES_DLL ALCBaselineModellingModel : public IALCBaselineModellingModel
  {
  public:
    // -- IALCBaselineModellingModel interface -----------------------------------------------------

    MatrixWorkspace_const_sptr data() const;

    void fit(IFunction_const_sptr function, const std::vector<Section> &sections);

    IFunction_const_sptr fittedFunction() const { return m_fittedFunction; }

    MatrixWorkspace_const_sptr correctedData() const;

    ITableWorkspace_sptr parameterTable() const { return m_parameterTable; }

    const std::vector<Section>& sections() const { return m_sections; }

    // -- End of IALCBaselineModellingModel interface ----------------------------------------------

    /// Set the data we should fit baseline for
    void setData(MatrixWorkspace_const_sptr data);

    /// Set the corrected data resulting from fit
    void setCorrectedData(MatrixWorkspace_const_sptr data);

    /// Export data + baseline + corrected data as a single workspace
    MatrixWorkspace_sptr exportWorkspace();

    /// Export sections used for the last fit as a table workspace
    ITableWorkspace_sptr exportSections();

    /// Exports baseline model as a table workspace
    ITableWorkspace_sptr exportModel();


  private:
    /// Data used for fitting
    MatrixWorkspace_const_sptr m_data;

    /// Result function of the last fit
    IFunction_const_sptr m_fittedFunction;

    /// Fit table containing parameters and errors
    ITableWorkspace_sptr m_parameterTable;

    /// Sections used for the last fit
    std::vector<Section> m_sections;

    // Setters for convenience
    void setFittedFunction(IFunction_const_sptr function);

    // Set errors in the ws after the fit
    void setErrorsAfterFit(MatrixWorkspace_sptr data);

    /// Disables points which shouldn't be used for fitting
    static void disableUnwantedPoints(MatrixWorkspace_sptr ws, const std::vector<Section>& sections);

    /// Enable previously disabled points
    static void enableDisabledPoints(MatrixWorkspace_sptr destWs, MatrixWorkspace_const_sptr sourceWs);

  };

} // namespace CustomInterfaces
} // namespace MantidQt

#endif  /* MANTID_CUSTOMINTERFACES_ALCBASELINEMODELLINGMODEL_H_ */

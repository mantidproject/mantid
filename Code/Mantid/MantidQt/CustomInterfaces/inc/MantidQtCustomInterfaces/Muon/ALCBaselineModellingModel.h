#ifndef MANTID_CUSTOMINTERFACES_ALCBASELINEMODELLINGMODEL_H_
#define MANTID_CUSTOMINTERFACES_ALCBASELINEMODELLINGMODEL_H_

#include "MantidKernel/System.h"
#include "MantidQtCustomInterfaces/DllConfig.h"

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

    MatrixWorkspace_const_sptr data() const { return m_data; }

    void fit(IFunction_const_sptr function, const std::vector<Section> &sections);

    IFunction_const_sptr fittedFunction() const { return m_fittedFunction; }

    MatrixWorkspace_const_sptr correctedData() const { return m_correctedData; }

    const std::vector<Section>& sections() const { return m_sections; }

    // -- End of IALCBaselineModellingModel interface ----------------------------------------------

    /// Set the data we should fit baseline for
    void setData(MatrixWorkspace_const_sptr data);

    /// Export data + baseline + corrected data as a single workspace
    MatrixWorkspace_sptr exportWorkspace();

    /// Export sections used for the last fit as a table workspace
    ITableWorkspace_sptr exportSections();

    /// Exports baseline model as a table workspace
    ITableWorkspace_sptr exportModel();


  private:
    /// Data to use for fitting
    MatrixWorkspace_const_sptr m_data;

    /// Corrected data of the last fit
    MatrixWorkspace_const_sptr m_correctedData;

    /// Result function of the last fit
    IFunction_const_sptr m_fittedFunction;

    /// Sections used for the last fit
    std::vector<Section> m_sections;

    // Setters for convenience
    void setCorrectedData(MatrixWorkspace_const_sptr data);
    void setFittedFunction(IFunction_const_sptr function);

    /// Disables points which shouldn't be used for fitting
    static void disableUnwantedPoints(MatrixWorkspace_sptr ws, const std::vector<Section>& sections);

  };

} // namespace CustomInterfaces
} // namespace MantidQt

#endif  /* MANTID_CUSTOMINTERFACES_ALCBASELINEMODELLINGMODEL_H_ */

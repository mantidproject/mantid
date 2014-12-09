#ifndef MANTIDQT_CUSTOMINTERFACES_ALCBASELINEMODELLINGPRESENTER_H_
#define MANTIDQT_CUSTOMINTERFACES_ALCBASELINEMODELLINGPRESENTER_H_

#include "MantidKernel/System.h"

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Muon/IALCBaselineModellingView.h"
#include "MantidQtCustomInterfaces/Muon/IALCBaselineModellingModel.h"

#include <QObject>

namespace MantidQt
{
namespace CustomInterfaces
{

  /** ALCBaselineModellingPresenter : Presenter for ALC Baseline Modelling step
    
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
  class MANTIDQT_CUSTOMINTERFACES_DLL ALCBaselineModellingPresenter : public QObject
  {
    Q_OBJECT

  public:
    ALCBaselineModellingPresenter(IALCBaselineModellingView* view, IALCBaselineModellingModel* model);

    void initialize();

  private slots:
    /// Perform a fit
    void fit();

    /// Add a new section
    void addSection();

    /// Remove existing section
    void removeSection(int row);

    /// Called when one of sections is modified
    void onSectionRowModified(int row);

    /// Called when on of section selectors is modified
    void onSectionSelectorModified(int index);

    /// Updates data curve from the model
    void updateDataCurve();

    /// Updates corrected data curve from the model
    void updateCorrectedCurve();

    /// Updated baseline curve from the model
    void updateBaselineCurve();

    /// Updates function in the view from the model
    void updateFunction();

    /// Removes all section rows / section selectors
    void clearSections();

  private:
    /// Associated view
    IALCBaselineModellingView* const m_view;

    /// Associated model
    IALCBaselineModellingModel* const m_model;
  };

} // namespace CustomInterfaces
} // namespace MantidQt

#endif  /* MANTIDQT_CUSTOMINTERFACES_ALCBASELINEMODELLINGPRESENTER_H_ */

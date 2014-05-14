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
    
    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    /// Set the data we should fit baseline for
    void setData(MatrixWorkspace_const_sptr data);

    /// A model used by presenter
    const IALCBaselineModellingModel& model() const { return *m_model; }

    /// Export data + baseline + corrected data as a single workspace
    MatrixWorkspace_sptr exportWorkspace();

    /// Export sections used for the last fit as a table workspace
    ITableWorkspace_sptr exportSections();

    /// Exports baseline model as a table workspace
    ITableWorkspace_sptr exportModel();

  private slots:
    /// Perform a fit
    void fit();

    /// Add a new section
    void addSection();

    /// Remove existing section
    void removeSection(size_t index);

    /// Called when one of sections is modified
    void onSectionModified(size_t index, double min, double max);

    /// Called when section selector is modified
    void onSectionSelectorModified(size_t index, double min, double max);

  private:
    /// Associated view
    IALCBaselineModellingView* const m_view;

    /// Associated model
    IALCBaselineModellingModel* const m_model;
  };

} // namespace CustomInterfaces
} // namespace MantidQt

#endif  /* MANTIDQT_CUSTOMINTERFACES_ALCBASELINEMODELLINGPRESENTER_H_ */

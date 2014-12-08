#ifndef MANTIDQT_CUSTOMINTERFACES_ALCINTERFACE_H_
#define MANTIDQT_CUSTOMINTERFACES_ALCINTERFACE_H_

#include "MantidKernel/System.h"

#include "MantidQtCustomInterfaces/DllConfig.h"

#include "MantidQtCustomInterfaces/Muon/ALCDataLoadingPresenter.h"
#include "MantidQtCustomInterfaces/Muon/ALCBaselineModellingPresenter.h"
#include "MantidQtCustomInterfaces/Muon/ALCPeakFittingPresenter.h"

#include "MantidQtCustomInterfaces/Muon/ALCBaselineModellingModel.h"
#include "MantidQtCustomInterfaces/Muon/ALCPeakFittingModel.h"

#include "MantidQtAPI/UserSubWindow.h"

#include "ui_ALCInterface.h"

#include <qwt_symbol.h>

namespace MantidQt
{
namespace CustomInterfaces
{

  /** ALCInterface : Custom interface for Avoided Level Crossing analysis
    
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
  class MANTIDQT_CUSTOMINTERFACES_DLL ALCInterface : public API::UserSubWindow
  {
    Q_OBJECT

  public:
    ALCInterface(QWidget* parent = 0);

    static std::string name() { return "ALC"; }
    static QString categoryInfo() { return "Muon"; }

  protected:
    void initLayout();

  private slots:
    void nextStep();
    void previousStep();

    void switchStep(int newStepIndex);

    void exportResults();

  private:

    /// UI form
    Ui::ALCInterface m_ui;

    // Step presenters
    ALCDataLoadingPresenter* m_dataLoading;
    ALCBaselineModellingPresenter* m_baselineModelling;
    ALCPeakFittingPresenter* m_peakFitting;

    // Models
    ALCBaselineModellingModel* m_baselineModellingModel;
    ALCPeakFittingModel* m_peakFittingModel;

    /// Name for every step for labels
    static const QStringList STEP_NAMES;

    /// Format of the label at the bottom
    static const QString LABEL_FORMAT;
  };

} // namespace CustomInterfaces
} // namespace MantidQt

#endif  /* MANTIDQT_CUSTOMINTERFACES_ALCINTERFACE_H_ */

#ifndef MANTIDQT_CUSTOMINTERFACES_ALCDATALOADINGVIEW_H_
#define MANTIDQT_CUSTOMINTERFACES_ALCDATALOADINGVIEW_H_

#include "MantidKernel/System.h"

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Muon/IALCDataLoadingView.h"
#include "MantidQtCustomInterfaces/Muon/ALCDataLoadingPresenter.h"

#include "ui_ALCDataLoading.h"

namespace MantidQt
{
namespace CustomInterfaces
{

  /** ALCDataLoadingView : ALC Data Loading view interface implementation using Qt widgets
    
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
  /**
   *
   */
  class MANTIDQT_CUSTOMINTERFACES_DLL ALCDataLoadingView : public IALCDataLoadingView
  {
  public:
    ALCDataLoadingView(QWidget* widget);

    std::string firstRun();
    std::string lastRun();
    std::string log();

    void displayData(MatrixWorkspace_const_sptr data);
    void displayError(const std::string &error);

  private:
    ALCDataLoadingPresenter m_dataLoading;
    Ui::ALCDataLoading m_ui;
    QWidget* const m_widget;
  };

} // namespace CustomInterfaces
} // namespace MantidQt

#endif  /* MANTIDQT_CUSTOMINTERFACES_ALCDATALOADINGVIEW_H_ */

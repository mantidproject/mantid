#ifndef MANTIDQT_CUSTOMINTERFACES_ALCDATALOADINGVIEW_H_
#define MANTIDQT_CUSTOMINTERFACES_ALCDATALOADINGVIEW_H_

#include "MantidKernel/System.h"

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Muon/IALCDataLoadingView.h"
#include "MantidQtCustomInterfaces/Muon/ALCDataLoadingPresenter.h"

#include "ui_ALCDataLoadingView.h"

#include <qwt_plot_curve.h>

namespace MantidQt
{
namespace CustomInterfaces
{

  /** ALCDataLoadingView : ALC Data Loading view interface implementation using Qt widgets
    
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
  /**
   *
   */
  class MANTIDQT_CUSTOMINTERFACES_DLL ALCDataLoadingView : public IALCDataLoadingView
  {
  public:
    ALCDataLoadingView(QWidget* widget);

    // -- IALCDataLoadingView interface ------------------------------------------------------------

    void initialize();

    std::string firstRun() const;
    std::string lastRun() const;
    std::string log() const;
    std::string deadTimeType() const;
    std::string deadTimeFile() const;
    std::string detectorGroupingType() const;
    std::string getForwardGrouping() const;
    std::string getBackwardGrouping() const;
    std::string calculationType() const;
    boost::optional< std::pair<double,double> > timeRange() const;

    void setDataCurve(const QwtData& data);
    void displayError(const std::string &error);
    void setAvailableLogs(const std::vector<std::string> &logs);
    void setAvailablePeriods(const std::vector<std::string> &periods);
    void setWaitingCursor();
    void restoreCursor();

    // -- End of IALCDataLoadingView interface -----------------------------------------------------

  private:
    /// UI form
    Ui::ALCDataLoadingView m_ui;

    /// The widget used
    QWidget* const m_widget;

    /// Loaded data curve
    QwtPlotCurve* m_dataCurve;
  };

} // namespace CustomInterfaces
} // namespace MantidQt

#endif  /* MANTIDQT_CUSTOMINTERFACES_ALCDATALOADINGVIEW_H_ */

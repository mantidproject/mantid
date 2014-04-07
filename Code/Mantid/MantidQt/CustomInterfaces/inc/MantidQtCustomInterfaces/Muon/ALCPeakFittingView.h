#ifndef MANTIDQT_CUSTOMINTERFACES_ALCPEAKFITTINGVIEW_H_
#define MANTIDQT_CUSTOMINTERFACES_ALCPEAKFITTINGVIEW_H_

#include "MantidKernel/System.h"

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Muon/IALCPeakFittingView.h"

#include "ui_ALCPeakFittingView.h"

#include <QWidget>
#include <qwt_plot_curve.h>

namespace MantidQt
{
namespace CustomInterfaces
{

  /** ALCPeakFittingView : Qt implementation of the ALC Peak Fitting step interface.
    
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
  class MANTIDQT_CUSTOMINTERFACES_DLL ALCPeakFittingView : public IALCPeakFittingView
  {
  public:
    ALCPeakFittingView(QWidget* widget);

    /// @see IALCPeakFittingView::peaks
    ListOfPeaks peaks() const;

  public slots:
    /// @see IALCPeakFittingView::initialize
    void initialize();

    /// @see IALCPeakFittingView::setData
    void setData(MatrixWorkspace_const_sptr data);

    /// @see IALCPeakFittingView::setPeaks
    void setPeaks(const ListOfPeaks &peaks);

  private:
    /// The widget used
    QWidget* const m_widget;

    /// UI form
    Ui::ALCPeakFittingView m_ui;

    /// Plot curves
    QwtPlotCurve *m_dataCurve, *m_peakCurve;
  };


} // namespace CustomInterfaces
} // namespace MantidQt

#endif  /* MANTIDQT_CUSTOMINTERFACES_ALCPEAKFITTINGVIEW_H_ */

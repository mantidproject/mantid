#ifndef MANTIDQT_CUSTOMINTERFACES_ALCPEAKFITTINGVIEW_H_
#define MANTIDQT_CUSTOMINTERFACES_ALCPEAKFITTINGVIEW_H_

#include "MantidKernel/System.h"

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Muon/IALCPeakFittingView.h"
#include "MantidQtMantidWidgets/PeakPicker.h"

#include "ui_ALCPeakFittingView.h"

#include <QWidget>
#include <qwt_plot_curve.h>

namespace MantidQt
{
namespace CustomInterfaces
{

  /** ALCPeakFittingView : Qt implementation of the ALC Peak Fitting step interface.
    
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
  class MANTIDQT_CUSTOMINTERFACES_DLL ALCPeakFittingView : public IALCPeakFittingView
  {
  public:
    ALCPeakFittingView(QWidget* widget);

    // -- IALCPeakFitting interface ----------------------------------------------------------------

    IFunction_const_sptr function(QString index) const;
    boost::optional<QString> currentFunctionIndex() const;
    IPeakFunction_const_sptr peakPicker() const;

  public slots:

    void initialize();
    void setDataCurve(const QwtData& data);
    void setFittedCurve(const QwtData& data);
    void setFunction(const IFunction_const_sptr& newFunction);
    void setParameter(const QString& funcIndex, const QString& paramName, double value);
    void setPeakPickerEnabled(bool enabled);
    void setPeakPicker(const IPeakFunction_const_sptr& peak);
    void help();

    // -- End of IALCPeakFitting interface ---------------------------------------------------------

  private:
    /// The widget used
    QWidget* const m_widget;

    /// UI form
    Ui::ALCPeakFittingView m_ui;

    /// Plot curves
    QwtPlotCurve *m_dataCurve, *m_fittedCurve;

    /// Peak picker tool - only one on the plot at any given moment
    MantidWidgets::PeakPicker* m_peakPicker;
  };


} // namespace CustomInterfaces
} // namespace MantidQt

#endif  /* MANTIDQT_CUSTOMINTERFACES_ALCPEAKFITTINGVIEW_H_ */

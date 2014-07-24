#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTCALIBRATION_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTCALIBRATION_H_

#include "MantidKernel/System.h"
#include "MantidQtCustomInterfaces/IndirectDataReductionTab.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"

namespace MantidQt
{
namespace CustomInterfaces
{
  /** IndirectCalibration

    @author Dan Nixon
    @date 23/07/2014

    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport IndirectCalibration : public IndirectDataReductionTab
  {
    Q_OBJECT

  public:
    IndirectCalibration(Ui::IndirectDataReduction& uiForm, QWidget * parent = 0);
    virtual ~IndirectCalibration();

    virtual void setup();
    virtual void run();
    virtual bool validate();

  private slots:
    void calPlotRaw();
    void calPlotEnergy();
    void calMinChanged(double);
    void calMaxChanged(double);
    void calUpdateRS(QtProperty*, double);
    void calSetDefaultResolution(Mantid::API::MatrixWorkspace_const_sptr ws);
    void resCheck(bool state); ///< handles checking/unchecking of "Create RES File" checkbox

  private:
    void createRESfile(const QString& file);

    /* Validators */
    QDoubleValidator *m_valDbl; ///< validator for double inputs

    QwtPlot* m_calCalPlot;
    QwtPlot* m_calResPlot;
    MantidWidgets::RangeSelector* m_calCalR1;
    MantidWidgets::RangeSelector* m_calCalR2;
    MantidWidgets::RangeSelector* m_calResR1;
    MantidWidgets::RangeSelector* m_calResR2;
    QwtPlotCurve* m_calCalCurve;
    QwtPlotCurve* m_calResCurve;
    QtTreePropertyBrowser* m_calCalTree;
    QtTreePropertyBrowser* m_calResTree;
    QMap<QString, QtProperty*> m_calCalProp;
    QMap<QString, QtProperty*> m_calResProp;
    QtDoublePropertyManager* m_calDblMng;
    QtGroupPropertyManager* m_calGrpMng;

  };
} // namespace CustomInterfaces
} // namespace Mantid

#endif //MANTIDQTCUSTOMINTERFACES_INDIRECTCALIBRATION_H_

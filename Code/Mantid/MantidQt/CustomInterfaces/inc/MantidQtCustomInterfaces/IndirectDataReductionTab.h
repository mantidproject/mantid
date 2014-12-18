#ifndef MANTID_CUSTOMINTERFACES_INDIRECTDATAREDUCTIONTAB_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTDATAREDUCTIONTAB_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/System.h"
#include "MantidQtAPI/AlgorithmRunner.h"
#include "MantidQtAPI/PythonRunner.h"
#include "MantidQtAPI/QwtWorkspaceSpectrumData.h"
#include "MantidQtCustomInterfaces/IndirectDataReduction.h"
#include "MantidQtMantidWidgets/RangeSelector.h"

#include <QDoubleValidator>
#include <QMap>
#include <QtIntPropertyManager>
#include <QtTreePropertyBrowser>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

// Suppress a warning coming out of code that isn't ours
#if defined(__INTEL_COMPILER)
  #pragma warning disable 1125
#elif defined(__GNUC__)
  #if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6 )
    #pragma GCC diagnostic push
  #endif
  #pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#include "DoubleEditorFactory.h"
#if defined(__INTEL_COMPILER)
  #pragma warning enable 1125
#elif defined(__GNUC__)
  #if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6 )
    #pragma GCC diagnostic pop
  #endif
#endif

namespace MantidQt
{
namespace CustomInterfaces
{
  /** IndirectDataReductionTab : TODO: DESCRIPTION
    
    @author Samuel Jackson
    @date 13/08/2013

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
  class DLLExport IndirectDataReductionTab : public QObject
  {
    Q_OBJECT

  public:
    IndirectDataReductionTab(Ui::IndirectDataReduction& uiForm, QObject * parent = 0);
    virtual ~IndirectDataReductionTab();

  public slots:
    void runTab();
    void setupTab();
    void validateTab();

  protected slots:
    /// Slot to handle when an algorithm finishes running
    virtual void algorithmFinished(bool error);

  protected:
    /// Run the load algorithm with the given file name, output name and spectrum range
    bool loadFile(const QString& filename, const QString& outputName, const int specMin = -1, const int specMax = -1);

    /// Function to get details about the instrument configuration defined on C2E tab
    std::map<QString, QString> getInstrumentDetails();

    /// Function to plot a workspace to the miniplot using a workspace name
    void plotMiniPlot(const QString& workspace, size_t index, const QString& plotID, const QString& curveID = "");
    /// Function to plot a workspace to the miniplot using a workspace pointer
    void plotMiniPlot(const Mantid::API::MatrixWorkspace_const_sptr & workspace, size_t wsIndex, const QString& plotID, const QString& curveID = "");
    /// Function to replot a miniplot
    void replot(const QString& plotID);

    /// Function to get the range of the curve displayed on the mini plot
    std::pair<double, double> getCurveRange(const QString& plotID);
    /// Function to set the range of an axis on a plot
    void setAxisRange(const QString& plotID, QwtPlot::Axis axis, std::pair<double, double> range);
    /// Function to autoscale a given axis based on the data in a curve
    void setXAxisToCurve(const QString& plotID, const QString& curveID);

    /// Function to set the range limits of the plot
    void setPlotRange(const QString& rsID, QtProperty* min, QtProperty* max, const std::pair<double, double>& bounds);
    /// Function to set the range selector on the mini plot
    void setMiniPlotGuides(const QString& rsID, QtProperty* lower, QtProperty* upper, const std::pair<double, double>& bounds);

    /// Function to run an algorithm on a seperate thread
    void runAlgorithm(const Mantid::API::IAlgorithm_sptr algorithm);

    /// Parent QWidget (if applicable)
    QWidget *m_parentWidget;

    /// Plot of the input
    std::map<QString, QwtPlot *> m_plots;
    /// Curve on the plot
    std::map<QString, QwtPlotCurve *> m_curves;
    /// Range selector widget for mini plot
    std::map<QString, MantidQt::MantidWidgets::RangeSelector *> m_rangeSelectors;
    /// Tree of the properties
    std::map<QString, QtTreePropertyBrowser *> m_propTrees;

    /// Internal list of the properties
    QMap<QString, QtProperty*> m_properties;

    /// Double manager to create properties
    QtDoublePropertyManager* m_dblManager;
    /// Boolean manager to create properties
    QtBoolPropertyManager* m_blnManager;
    /// Group manager to create properties
    QtGroupPropertyManager* m_grpManager;

    /// Double editor facotry for the properties browser
    DoubleEditorFactory* m_dblEdFac;
    /// Algorithm runner object to execute algorithms on a seperate thread from the gui
    MantidQt::API::AlgorithmRunner* m_algRunner;

    /// Use a Python runner for when we need the output of a script
    MantidQt::API::PythonRunner m_pythonRunner;

    /// Validator for int inputs
    QIntValidator *m_valInt;
    /// Validator for double inputs
    QDoubleValidator *m_valDbl;
    /// Validator for positive double inputs
    QDoubleValidator *m_valPosDbl;

  signals:
    /// Send signal to parent window to show a message box to user
    void showMessageBox(const QString& message);
    /// Run a python script
    void runAsPythonScript(const QString & code, bool no_output);

  private:
    /// Overidden by child class.
    virtual void setup() = 0;
    /// Overidden by child class.
    virtual void run() = 0;
    /// Overidden by child class.
    virtual bool validate() = 0;

  protected:
    Ui::IndirectDataReduction m_uiForm;

  };
} // namespace CustomInterfaces
} // namespace Mantid

#endif  /* MANTID_CUSTOMINTERFACES_INDIRECTDATAREDUCTIONTAB_H_ */

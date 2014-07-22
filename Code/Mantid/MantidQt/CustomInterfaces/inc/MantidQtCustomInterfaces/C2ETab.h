#ifndef MANTID_CUSTOMINTERFACES_C2ETAB_H_
#define MANTID_CUSTOMINTERFACES_C2ETAB_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/System.h"
#include "MantidQtAPI/AlgorithmRunner.h"
#include "MantidQtAPI/QwtWorkspaceSpectrumData.h"
#include "MantidQtCustomInterfaces/IndirectDataReduction.h"
#include "MantidQtMantidWidgets/RangeSelector.h"

#include <QMap>
#include <QDoubleValidator>
#include <QtDoublePropertyManager>
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


  /** C2ETab : TODO: DESCRIPTION
    
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
  class DLLExport C2ETab : public QWidget
  {
    Q_OBJECT

  public:
    C2ETab(Ui::IndirectDataReduction& uiForm, QWidget * parent = 0);
    virtual ~C2ETab();
    void runTab();
    void setupTab();
    void validateTab();

  protected slots:
    /// Slot to handle when an algorithm finishs running
    virtual void algorithmFinished(bool error);

  protected:
    // Run the load algorithm with the given file name and output name 
    bool loadFile(const QString& filename, const QString& outputName);
    /// Function to plot a workspace to the miniplot using a workspace name
    void plotMiniPlot(const QString& workspace, size_t index);
    /// Function to plot a workspace to the miniplot using a workspace pointer
    void plotMiniPlot(const Mantid::API::MatrixWorkspace_const_sptr & workspace, size_t wsIndex);
    /// Function to get the range of the curve displayed on the mini plot
    std::pair<double,double> getCurveRange();
    /// Function to set the range limits of the plot
    void setPlotRange(QtProperty* min, QtProperty* max, const std::pair<double, double>& bounds);
    /// Function to set the range selector on the mini plot
    void setMiniPlotGuides(QtProperty* lower, QtProperty* upper, const std::pair<double, double>& bounds);
    /// Function to run an algorithm on a seperate thread
    void runAlgorithm(const Mantid::API::Algorithm_sptr algorithm);

    /// Plot of the input
    QwtPlot* m_plot;
    /// Curve on the plot
    QwtPlotCurve* m_curve;
    /// Range selector widget for mini plot
    MantidQt::MantidWidgets::RangeSelector* m_rangeSelector;
    /// Tree of the properties
    QtTreePropertyBrowser* m_propTree;
    /// Internal list of the properties
    QMap<QString, QtProperty*> m_properties;
    /// Double manager to create properties
    QtDoublePropertyManager* m_dblManager;
    /// Double editor facotry for the properties browser
    DoubleEditorFactory* m_dblEdFac;
    /// Algorithm runner object to execute algorithms on a seperate thread from the gui
    MantidQt::API::AlgorithmRunner* m_algRunner;

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

#endif  /* MANTID_CUSTOMINTERFACES_C2ETAB_H_ */

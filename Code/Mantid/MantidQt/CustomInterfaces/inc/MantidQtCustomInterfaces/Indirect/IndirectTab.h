#ifndef MANTID_CUSTOMINTERFACES_INDIRECTTAB_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTTAB_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/System.h"
#include "MantidQtAPI/AlgorithmRunner.h"
#include "MantidQtAPI/BatchAlgorithmRunner.h"
#include "MantidQtAPI/PythonRunner.h"
#include "MantidQtAPI/QwtWorkspaceSpectrumData.h"
#include "MantidQtMantidWidgets/RangeSelector.h"

#include <QDoubleValidator>
#include <QMap>
#include <QPair>
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
  /** IndirectTab

    Provided common functionality of all indirect interface tabs.

    @author Dan Nixon
    @date 08/10/2014

    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class DLLExport IndirectTab : public QObject
  {
    Q_OBJECT

  public:
    IndirectTab(QObject* parent = 0);
    virtual ~IndirectTab();

  public slots:
    void runTab();
    void setupTab();
    bool validateTab();
    void exportPythonScript();

  protected slots:
    /// Slot to handle when an algorithm finishes running
    virtual void algorithmFinished(bool error);

  protected:
    /// Run the load algorithm with the given file name, output name and spectrum range
    bool loadFile(const QString& filename, const QString& outputName, const int specMin = -1, const int specMax = -1);

    /// Add a SaveNexusProcessed step to the batch queue
    void addSaveWorkspaceToQueue(const QString & wsName, const QString & filename = "");

    /// Plot a spectrum plot with a given spectrum index
    void plotSpectrum(const QStringList & workspaceNames, int specIndex = 0);
    /// Plot a spectrum plot of a given workspace
    void plotSpectrum(const QString & workspaceName, int specIndex = 0);

    /// Plot a spectrum plot with a given spectra range
    void plotSpectrum(const QStringList & workspaceNames, int specStart, int specEnd);
    /// Plot a spectrum plot with a given spectra range of a given workspace
    void plotSpectrum(const QString & workspaceName, int specStart, int specEnd);

    /// Plot a time bin plot given a list of workspace names
    void plotTimeBin(const QStringList & workspaceNames, int specIndex = 0);
    /// Plot a time bin plot of a given workspace
    void plotTimeBin(const QString & workspaceName, int specIndex = 0);

    /// Plot a contour plot of a given workspace
    void plot2D(const QString & workspaceName);

    /// Function to set the range limits of the plot
    void setPlotPropertyRange(MantidQt::MantidWidgets::RangeSelector * rs,
                              QtProperty* min, QtProperty* max,
                              const QPair<double, double> & bounds);
    /// Function to set the range selector on the mini plot
    void setRangeSelector(MantidQt::MantidWidgets::RangeSelector * rs,
                          QtProperty* lower, QtProperty* upper,
                          const QPair<double, double> & bounds);

    /// Function to get energy mode from a workspace
    std::string getEMode(Mantid::API::MatrixWorkspace_sptr ws);

    /// Function to get eFixed from a workspace
    double getEFixed(Mantid::API::MatrixWorkspace_sptr ws);

    /// Function to run an algorithm on a seperate thread
    void runAlgorithm(const Mantid::API::IAlgorithm_sptr algorithm);

    QString runPythonCode(QString vode, bool no_output = false);

    /// Parent QWidget (if applicable)
    QWidget *m_parentWidget;

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

    /// Algorithm runner object to execute chains algorithms on a seperate thread from the GUI
    MantidQt::API::BatchAlgorithmRunner *m_batchAlgoRunner;

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
    void runAsPythonScript(const QString & code, bool noOutput = false);

  protected:
    /// Overidden by child class.
    virtual void setup() = 0;
    /// Overidden by child class.
    virtual void run() = 0;
    /// Overidden by child class.
    virtual bool validate() = 0;

    Mantid::Kernel::DateAndTime m_tabStartTime;
    Mantid::Kernel::DateAndTime m_tabEndTime;
    std::string m_pythonExportWsName;

  };
} // namespace CustomInterfaces
} // namespace Mantid

#endif  /* MANTID_CUSTOMINTERFACES_INDIRECTTAB_H_ */

// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_INDIRECTTAB_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTTAB_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidQtWidgets/Common/AlgorithmRunner.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "MantidQtWidgets/Common/PythonRunner.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/QtIntPropertyManager"
#include "MantidQtWidgets/Common/QtPropertyBrowser/QtTreePropertyBrowser"
#include "MantidQtWidgets/Plotting/PreviewPlot.h"
#include "MantidQtWidgets/Plotting/RangeSelector.h"

#include <QDoubleValidator>
#include <QMap>
#include <QPair>

#include <algorithm>
#include <map>
#include <unordered_map>

// Suppress a warning coming out of code that isn't ours
#if defined(__INTEL_COMPILER)
#pragma warning disable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h"
#if defined(__INTEL_COMPILER)
#pragma warning enable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
#endif

namespace MantidQt {
namespace CustomInterfaces {
/** IndirectTab

  Provided common functionality of all indirect interface tabs.

  @author Dan Nixon
  @date 08/10/2014
*/
class DLLExport IndirectTab : public QObject {
  Q_OBJECT

public:
  IndirectTab(QObject *parent = nullptr);
  ~IndirectTab() override;

  /// Get the suffixes used for an interface from the xml file
  QStringList getExtensions(std::string const &interfaceName) const;
  QStringList getCalibrationExtensions(std::string const &interfaceName) const;
  QStringList getSampleFBSuffixes(std::string const &interfaceName) const;
  QStringList getSampleWSSuffixes(std::string const &interfaceName) const;
  QStringList getVanadiumFBSuffixes(std::string const &interfaceName) const;
  QStringList getVanadiumWSSuffixes(std::string const &interfaceName) const;
  QStringList getResolutionFBSuffixes(std::string const &interfaceName) const;
  QStringList getResolutionWSSuffixes(std::string const &interfaceName) const;
  QStringList getCalibrationFBSuffixes(std::string const &interfaceName) const;
  QStringList getCalibrationWSSuffixes(std::string const &interfaceName) const;
  QStringList getContainerFBSuffixes(std::string const &interfaceName) const;
  QStringList getContainerWSSuffixes(std::string const &interfaceName) const;
  QStringList getCorrectionsFBSuffixes(std::string const &interfaceName) const;
  QStringList getCorrectionsWSSuffixes(std::string const &interfaceName) const;

  /// Allows the user to turn the plotting of error bars off and on
  void setPlotErrorBars(bool errorBars);

  /// Plot a spectrum plot of a given workspace
  void plotSpectrum(const QString &workspaceName, const int &spectraIndex = 0);

public slots:
  void runTab();
  void setupTab();
  bool validateTab();
  void exportPythonScript();

protected slots:
  /// Slot to handle when an algorithm finishes running
  virtual void algorithmFinished(bool error);

protected:
  /// Run the load algorithms
  bool loadFile(const QString &filename, const QString &outputName,
                const int specMin = -1, const int specMax = -1,
                bool loadHistory = true);

  /// Add a SaveNexusProcessed step to the batch queue
  void addSaveWorkspaceToQueue(const std::string &wsName,
                               const std::string &filename = "");
  void addSaveWorkspaceToQueue(const QString &wsName,
                               const QString &filename = "");

  /// Gets the workspace suffix of a workspace name
  QString getWorkspaceSuffix(const QString &wsName);
  /// Gets the base name of a workspace
  QString getWorkspaceBasename(const QString &wsName);
  /// Plot multiple spectra from multiple workspaces
  void plotMultipleSpectra(const QStringList &workspaceNames,
                           const std::vector<int> &workspaceIndices);
  /// Plot a spectrum plot with a given ws index
  void plotSpectrum(const QStringList &workspaceNames,
                    const int &spectraIndex = 0);

  /// Plot a spectrum plot with a given spectra range
  void plotSpectrum(const QStringList &workspaceNames, int specStart,
                    int specEnd);
  /// Plot a spectrum plot with a given spectra range of a given workspace
  void plotSpectrum(const QString &workspaceName, int specStart, int specEnd);

  /// Plot a spectrum plot with a given set of spectra
  void plotSpectra(const QStringList &workspaceNames,
                   const std::vector<int> &wsIndices);

  /// Plot a spectrum plot with a given set of spectra of a given workspace
  void plotSpectra(const QString &workspaceName,
                   const std::vector<int> &wsIndices);

  /// Plot multiple spectra in a tiled plot
  void plotTiled(std::string const &workspaceName, std::size_t const &fromIndex,
                 std::size_t const &toIndex);

  /// Plot a time bin plot given a list of workspace names
  void plotTimeBin(const QStringList &workspaceNames, int binIndex = 0);
  /// Plot a time bin plot of a given workspace
  void plotTimeBin(const QString &workspaceName, int binIndex = 0);

  /// Plot a contour plot of a given workspace
  void plot2D(const QString &workspaceName);

  /// Resizes the specified plot range
  void resizePlotRange(MantidWidgets::PreviewPlot *preview,
                       QPair<double, double> range);

  /// Extracts the labels from the axis at the specified index in the
  /// specified workspace.
  std::unordered_map<std::string, size_t>
  extractAxisLabels(Mantid::API::MatrixWorkspace_const_sptr workspace,
                    const size_t &axisIndex) const;

  /// Function to set the range limits of the plot
  void setPlotPropertyRange(MantidQt::MantidWidgets::RangeSelector *rs,
                            QtProperty *min, QtProperty *max,
                            const QPair<double, double> &bounds);
  /// Function to set the range selector on the mini plot
  void setRangeSelector(MantidQt::MantidWidgets::RangeSelector *rs,
                        QtProperty *lower, QtProperty *upper,
                        const QPair<double, double> &bounds);

  /// Function to get energy mode from a workspace
  std::string getEMode(Mantid::API::MatrixWorkspace_sptr ws);

  /// Function to get eFixed from a workspace
  double getEFixed(Mantid::API::MatrixWorkspace_sptr ws);

  /// Function to read an instrument's resolution from the IPF using a string
  bool getResolutionRangeFromWs(const QString &filename,
                                QPair<double, double> &res);

  /// Function to read an instrument's resolution from the IPF using a workspace
  /// pointer
  bool getResolutionRangeFromWs(Mantid::API::MatrixWorkspace_const_sptr ws,
                                QPair<double, double> &res);

  /// Converts a standard vector of standard strings to a QVector of QStrings.
  QVector<QString>
  convertStdStringVector(const std::vector<std::string> &stringVec) const;

  /// Function to run an algorithm on a seperate thread
  void runAlgorithm(const Mantid::API::IAlgorithm_sptr algorithm);

  QString runPythonCode(QString vode, bool no_output = false);

  /// Checks the ADS for a workspace named `workspaceName`,
  /// opens a warning box for plotting/saving if none found
  bool checkADSForPlotSaveWorkspace(const std::string &workspaceName,
                                    const bool plotting,
                                    const bool warn = true);

  /// Parent QWidget (if applicable)
  QWidget *m_parentWidget;

  /// Tree of the properties
  std::map<QString, QtTreePropertyBrowser *> m_propTrees;

  /// Internal list of the properties
  QMap<QString, QtProperty *> m_properties;

  /// Double manager to create properties
  QtDoublePropertyManager *m_dblManager;
  /// Boolean manager to create properties
  QtBoolPropertyManager *m_blnManager;
  /// Group manager to create properties
  QtGroupPropertyManager *m_grpManager;

  /// Double editor facotry for the properties browser
  DoubleEditorFactory *m_dblEdFac;

  /// Algorithm runner object to execute chains algorithms on a seperate thread
  /// from the GUI
  MantidQt::API::BatchAlgorithmRunner *m_batchAlgoRunner;

  /// Use a Python runner for when we need the output of a script
  MantidQt::API::PythonRunner m_pythonRunner;

  /// Plot error bars when plotting a spectrum
  bool m_plotErrorBars;

  /// Validator for int inputs
  QIntValidator *m_valInt;
  /// Validator for double inputs
  QDoubleValidator *m_valDbl;
  /// Validator for positive double inputs
  QDoubleValidator *m_valPosDbl;

signals:
  /// Send signal to parent window to show a message box to user
  void showMessageBox(const QString &message);
  /// Run a python script
  void runAsPythonScript(const QString &code, bool noOutput = false);

protected:
  /// Overidden by child class.
  virtual void setup() = 0;
  /// Overidden by child class.
  virtual void run() = 0;
  /// Overidden by child class.
  virtual bool validate() = 0;

  Mantid::Types::Core::DateAndTime m_tabStartTime;
  Mantid::Types::Core::DateAndTime m_tabEndTime;
  std::string m_pythonExportWsName;

private:
  std::string getInterfaceProperty(std::string const &interfaceName,
                                   std::string const &propertyName,
                                   std::string const &attribute) const;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_CUSTOMINTERFACES_INDIRECTTAB_H_ */

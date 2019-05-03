// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_CORRECTIONSTAB_H_
#define MANTIDQTCUSTOMINTERFACESIDA_CORRECTIONSTAB_H_

#include "IndirectTab.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

class QwtPlotCurve;
class QwtPlot;
class QSettings;
class QString;

namespace MantidQt {
namespace MantidWidgets {
class RangeSelector;
}
} // namespace MantidQt

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
#include "MantidQtWidgets/Common/QtPropertyBrowser/qteditorfactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qttreepropertybrowser.h"
#if defined(__INTEL_COMPILER)
#pragma warning enable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
#endif

namespace MantidQt {
namespace CustomInterfaces {

struct Densities {
  Densities() : m_massDensity(1.0), m_numberDensity(0.1){};

  void setMassDensity(double value) { m_massDensity = value; }
  void setNumberDensity(double value) { m_numberDensity = value; }
  double getMassDensity() const { return m_massDensity; }
  double getNumberDensity() const { return m_numberDensity; }
  std::string getMassDensityUnit() const { return " g/cm3"; }
  std::string getNumberDensityUnit() const { return " /A3"; }

private:
  double m_massDensity;
  double m_numberDensity;
};

class DLLExport CorrectionsTab : public IndirectTab {
  Q_OBJECT

public:
  /// Constructor
  CorrectionsTab(QWidget *parent = nullptr);

  /// Loads the tab's settings.
  void loadTabSettings(const QSettings &settings);

protected:
  /// Function to run a string as python code
  void runPythonScript(const QString &pyInput);
  /// Check the binning between two workspaces match
  bool
  checkWorkspaceBinningMatches(Mantid::API::MatrixWorkspace_const_sptr left,
                               Mantid::API::MatrixWorkspace_const_sptr right);
  /// Adds a unit conversion step to the algorithm queue
  std::string addConvertUnitsStep(Mantid::API::MatrixWorkspace_sptr ws,
                                  const std::string &unitID,
                                  const std::string &suffix = "UNIT",
                                  std::string eMode = "");
  /// Displays and logs the error for a workspace with an invalid type
  void displayInvalidWorkspaceTypeError(const std::string &workspaceName,
                                        Mantid::Kernel::Logger &log);

  /// DoubleEditorFactory
  DoubleEditorFactory *m_dblEdFac;
  /// QtCheckBoxFactory
  QtCheckBoxFactory *m_blnEdFac;

protected slots:
  /// Slot that can be called when a user eidts an input.
  void inputChanged();

private:
  /// Overidden by child class.
  void setup() override = 0;
  /// Overidden by child class.
  void run() override = 0;
  /// Overidden by child class.
  bool validate() override = 0;

  /// Overidden by child class.
  virtual void loadSettings(const QSettings &settings) = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_CORRECTIONSTAB_H_ */

// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Spectroscopy/InelasticTab.h"
#include "MantidQtWidgets/Spectroscopy/OutputWidget/OutputPlotOptionsPresenter.h"

#include "DllConfig.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

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
  Densities() : m_massDensity(1.0), m_numberDensity(0.1) {};

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

class MANTIDQT_INELASTIC_DLL CorrectionsTab : public InelasticTab {
  Q_OBJECT

public:
  /// Constructor
  CorrectionsTab(QWidget *parent = nullptr);
  /// Set the active workspaces used in the plotting options
  void setOutputPlotOptionsWorkspaces(std::vector<std::string> const &outputWorkspaces);
  /// Used to clear the workspaces held by the output plotting widget
  void clearOutputPlotOptionsWorkspaces();

  /// Loads the tab's settings.
  void loadTabSettings(const QSettings &settings);

  /// Prevent loading of data with incorrect naming
  void filterInputData(bool filter);
  void enableLoadHistoryProperty(bool doLoadHistory);

protected:
  /// Check the binning between two workspaces match
  bool checkWorkspaceBinningMatches(const Mantid::API::MatrixWorkspace_const_sptr &left,
                                    const Mantid::API::MatrixWorkspace_const_sptr &right);
  /// Adds a unit conversion step to the algorithm queue
  std::optional<std::string> addConvertUnitsStep(const Mantid::API::MatrixWorkspace_sptr &ws, const std::string &unitID,
                                                 const std::string &suffix = "UNIT", std::string eMode = "",
                                                 double eFixed = 0.0);
  /// Displays and logs the error for a workspace with an invalid type
  void displayInvalidWorkspaceTypeError(const std::string &workspaceName, Mantid::Kernel::Logger &log);

  /// DoubleEditorFactory
  DoubleEditorFactory *m_dblEdFac;
  /// QtCheckBoxFactory
  QtCheckBoxFactory *m_blnEdFac;

private:
  virtual void loadSettings(const QSettings &settings) = 0;
  virtual void setFileExtensionsByName(bool filter) = 0;
  virtual void setLoadHistory(bool doLoadHistory) { (void)doLoadHistory; };
};
} // namespace CustomInterfaces
} // namespace MantidQt

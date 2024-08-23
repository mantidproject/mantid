// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Spectroscopy/InelasticInterface.h"
#include "MantidQtWidgets/Spectroscopy/InelasticTab.h"

#include "ui_Corrections.h"

#include "MantidKernel/ConfigService.h"
#include <Poco/NObserver.h>

class DoubleEditorFactory;
class QtCheckBoxFactory;
class QtStringPropertyManager;

namespace MantidQt {
namespace CustomInterfaces {
// The assumption is made elsewhere that the ordering of these enums matches the
// ordering of the
// tabs as they appear in the interface itself.
enum CorrectionTabChoice { CONTAINER_SUBTRACTION, CALC_CORR, ABSORPTION_CORRECTIONS, APPLY_CORR };

// Forward Declaration
class CorrectionsTab;

/**
 * The Corrections class is the main class that handles the interface
 * and controls
 * its tabs.
 *
 * Is a friend to the CorrectionsTab class.
 */
class Corrections : public InelasticInterface {
  Q_OBJECT

  /// Allow CorrectionsTab to have access.
  friend class CorrectionsTab;

public:
  /// The name of the interface as registered into the factory
  static std::string name() { return "Corrections"; }
  // This interface's categories.
  static QString categoryInfo() { return "Inelastic"; }
  /// Default Constructor
  explicit Corrections(QWidget *parent = nullptr);

private:
  /// Initialize the layout
  void initLayout() override;
  /// Initialize Python-dependent sections
  void initLocalPython() override;
  /// Load the settings of the interface (and child tabs).
  void loadSettings();

  /// Called upon a close event.
  void closeEvent(QCloseEvent * /*unused*/) override;
  /// handle POCO event
  void handleDirectoryChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf);

private slots:
  /// Called when the user clicks the Py button
  void exportTabPython();

private:
  std::string documentationPage() const override;

  void applySettings(std::map<std::string, QVariant> const &settings) override;

  /// UI form containing all Qt elements.
  Ui::Corrections m_uiForm;

  /// Change Observer for ConfigService (monitors user directories)
  Poco::NObserver<Corrections, Mantid::Kernel::ConfigValChangeNotification> m_changeObserver;

  /// Map of unsigned int (TabChoice enum values) to tabs.
  std::map<unsigned int, CorrectionsTab *> m_tabs;
};
} // namespace CustomInterfaces
} // namespace MantidQt

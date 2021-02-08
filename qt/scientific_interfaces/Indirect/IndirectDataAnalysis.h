// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "ui_IndirectDataAnalysis.h"

#include "IndirectInterface.h"
#include "IndirectTab.h"

#include "MantidKernel/ConfigService.h"
#include <Poco/NObserver.h>

class DoubleEditorFactory;
class QtCheckBoxFactory;
class QtStringPropertyManager;

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
// The assumption is made elsewhere that the ordering of these enums matches the
// ordering of the
// tabs as they appear in the interface itself.
enum IDATabChoice { ELWIN, MSD_FIT, IQT, IQT_FIT, CONV_FIT, FQ_FIT };

// Number of decimal places in property browsers.
static const unsigned int NUM_DECIMALS = 6;

// Forward Declaration
class IndirectDataAnalysisTab;

/**
 * The IndirectDataAnalysis class is the main class that handles the interface
 *and controls
 * its tabs.
 *
 * Is a friend to the IndirectDataAnalysisTab class.
 */
class IndirectDataAnalysis : public IndirectInterface {
  Q_OBJECT

  /// Allow IndirectDataAnalysisTab to have access.
  friend class IndirectDataAnalysisTab;

public:
  /// The name of the interface as registered into the factory
  static std::string name() { return "Data Analysis"; }
  /// This interface's categories.
  static QString categoryInfo() { return "Indirect"; }
  /// Default Constructor
  explicit IndirectDataAnalysis(QWidget *parent = nullptr);

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
  void
  handleDirectoryChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf);

private slots:
  /// Sets the active workspace in the selected tab
  void tabChanged(int index);
  /// Called when the user clicks the Py button
  void exportTabPython();

private:
  std::string documentationPage() const override;

  void applySettings(std::map<std::string, QVariant> const &settings) override;

  /// UI form containing all Qt elements.
  Ui::IndirectDataAnalysis m_uiForm;
  /// The settings group
  QString m_settingsGroup;
  /// Integer validator
  QIntValidator *m_valInt;
  /// Double validator
  QDoubleValidator *m_valDbl;

  /// Change Observer for ConfigService (monitors user directories)
  Poco::NObserver<IndirectDataAnalysis,
                  Mantid::Kernel::ConfigValChangeNotification>
      m_changeObserver;

  /// Map of unsigned int (TabChoice enum values) to tabs.
  std::map<unsigned int, IndirectDataAnalysisTab *> m_tabs;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

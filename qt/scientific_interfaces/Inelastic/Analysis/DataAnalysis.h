// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/IndirectInterface.h"
#include "Common/IndirectTab.h"
#include "DllConfig.h"
#include "ui_DataAnalysis.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
// The assumption is made elsewhere that the ordering of these enums matches the
// ordering of the
// tabs as they appear in the interface itself.
enum IDATabChoice { MSD_FIT, IQT_FIT, CONV_FIT, FQ_FIT };

// Number of decimal places in property browsers.
static const unsigned int NUM_DECIMALS = 6;

// Forward Declaration
class DataAnalysisTab;

/**
 * The DataAnalysis class is the main class that handles the interface
 *and controls
 * its tabs.
 */
class MANTIDQT_INELASTIC_DLL DataAnalysis final : public IndirectInterface {
  Q_OBJECT

public:
  /// The name of the interface as registered into the factory
  static std::string name() { return "Data Analysis"; }
  /// This interface's categories.
  static QString categoryInfo() { return "Inelastic"; }
  /// Default Constructor
  explicit DataAnalysis(QWidget *parent = nullptr);

private:
  /// Initialize the layout
  void initLayout() override;

private slots:
  /// Called when the user clicks the Py button
  void exportTabPython();

private:
  std::string documentationPage() const override;

  void applySettings(std::map<std::string, QVariant> const &settings) override;

  /// UI form containing all Qt elements.
  Ui::DataAnalysis m_uiForm;
  /// The settings group
  QString m_settingsGroup;

  /// Map of unsigned int (TabChoice enum values) to tabs.
  std::map<unsigned int, DataAnalysisTab *> m_tabs;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

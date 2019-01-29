// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTDATAMANIPULATION_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTDATAMANIPULATION_H_

#include "ui_IndirectDataManipulation.h"

#include "MantidQtWidgets/Common/UserSubWindow.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

enum IDMTabChoice { SYMMETRISE, SQW2, MOMENTS2, ELWIN, IQT };

class IndirectDataManipulation : public MantidQt::API::UserSubWindow {
  Q_OBJECT

  friend class IndirectDataManipulationTab;

public:
  explicit IndirectDataManipulation(QWidget *parent = nullptr);

  static std::string name() { return "Data Manipulation"; }
  static QString categoryInfo() { return "Indirect"; }

private slots:
  void handleHelp();
  void handleExportToPython();
  void handleManageDirectories();
  void showMessageBox(QString const &message);

private:
  void initLayout() override;

  std::map<std::size_t, IndirectDataManipulationTab *> m_tabs;

  Ui::IndirectDataManipulation m_uiForm;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACES_INDIRECTDATAMANIPULATION_H_ */

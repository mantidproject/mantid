// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_INDIRECTSETTINGS_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTSETTINGS_H_

#include "ui_IndirectSettings.h"

#include "IndirectSettingsPresenter.h"

#include "DllConfig.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL IndirectSettings
    : public MantidQt::API::UserSubWindow {
  Q_OBJECT

public:
  IndirectSettings(QWidget *parent = nullptr);
  ~IndirectSettings() = default;

  static std::string name() { return "Settings"; }
  static QString categoryInfo() { return "Indirect"; }

private slots:
  void closeSettings();

private:
  void initLayout() override;

  std::unique_ptr<IndirectSettingsPresenter> m_presenter;
  Ui::IndirectSettings m_uiForm;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_CUSTOMINTERFACES_INDIRECTSETTINGS_H_ */

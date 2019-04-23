// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectSettings.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
DECLARE_SUBWINDOW(IndirectSettings)

IndirectSettings::IndirectSettings(QWidget *parent)
    : MantidQt::API::UserSubWindow(parent) {
  m_uiForm.setupUi(this);
}

void IndirectSettings::initLayout() {
  m_presenter = Mantid::Kernel::make_unique<IndirectSettingsPresenter>(this);

  auto centralWidget = m_uiForm.centralWidget->layout();
  centralWidget->addWidget(m_presenter->getView());

  connect(m_presenter.get(), SIGNAL(closeSettings()), this,
          SLOT(closeSettings()));
}

void IndirectSettings::closeSettings() { this->close(); }

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

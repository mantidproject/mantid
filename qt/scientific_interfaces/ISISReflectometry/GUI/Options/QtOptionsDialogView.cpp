// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtOptionsDialogView.h"
#include "OptionsDialogModel.h"
#include "OptionsDialogPresenter.h"
#include <QCloseEvent>
#include <QPushButton>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

QtOptionsDialogView::QtOptionsDialogView(QWidget *parent) {
  Q_UNUSED(parent);
  initLayout();
  initBindings();
}

QtOptionsDialogView::~QtOptionsDialogView() {}

/** Initialise the ui */
void QtOptionsDialogView::initLayout() {
  m_ui.setupUi(this);
  connect(m_ui.buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this,
          SLOT(notifySaveOptions()));
  connect(m_ui.buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()),
          this, SLOT(notifyLoadOptions()));
}

/** Bind options to their widgets */
void QtOptionsDialogView::initBindings() {
  m_bindings.clear();

  // Check all the widgets for the "reflOptionName" property.
  // If it exists, bind the named option to that widget.
  QList<QWidget *> widgets = findChildren<QWidget *>();
  for (auto &widget : widgets) {
    QVariant binding = widget->property("reflOptionName");
    if (binding.isValid())
      m_bindings[binding.toString()] = widget->objectName();
  }
}

/** Saves the currently configured options to the presenter
 *
 * @param boolOptions A map containing bool options
 * @param intOptions A map containing int options
 * @return void
 *
 */
void QtOptionsDialogView::getOptions(std::map<std::string, bool> &boolOptions,
                                     std::map<std::string, int> &intOptions) {
  // Iterate through all our bound widgets, pushing their value into the options
  // map
  for (const auto &binding : m_bindings) {
    QString widgetName = binding.second;
    if (widgetName.isEmpty())
      continue;

    QCheckBox *checkbox = findChild<QCheckBox *>(widgetName);
    if (checkbox) {
      boolOptions[binding.first.toStdString()] = checkbox->isChecked();
      continue;
    }

    QSpinBox *spinbox = findChild<QSpinBox *>(widgetName);
    if (spinbox) {
      intOptions[binding.first.toStdString()] = spinbox->value();
      continue;
    }
  }
}

/** Sets the ui to match the presenter's options
 *
 * @param boolOptions A map to store bool options
 * @param intOptions A map to store int options
 * @return void
 *
 */
void QtOptionsDialogView::setOptions(std::map<std::string, bool> &boolOptions,
                                     std::map<std::string, int> &intOptions) {
  // Set the values from the options
  for (auto &boolOption : boolOptions) {
    QString widgetName = m_bindings[QString::fromStdString(boolOption.first)];
    if (widgetName.isEmpty())
      continue;

    QCheckBox *checkbox = findChild<QCheckBox *>(widgetName);
    if (checkbox) {
      checkbox->setChecked(boolOption.second);
      continue;
    }
  }
  for (auto &intOption : intOptions) {
    QString widgetName = m_bindings[QString::fromStdString(intOption.first)];
    if (widgetName.isEmpty())
      continue;

    QSpinBox *spinbox = findChild<QSpinBox *>(widgetName);
    if (spinbox) {
      spinbox->setValue(intOption.second);
      continue;
    }
  }
}

void QtOptionsDialogView::subscribe(OptionsDialogViewSubscriber *notifyee) {
  m_notifyee = notifyee;
}

void QtOptionsDialogView::notifyLoadOptions() { m_notifyee->loadOptions(); }
void QtOptionsDialogView::notifySaveOptions() { m_notifyee->saveOptions(); }

void QtOptionsDialogView::closeEvent(QCloseEvent *event) {
  Q_UNUSED(event);
  notifyLoadOptions();
  this->reject();
}

void QtOptionsDialogView::show() { QDialog::exec(); }

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt

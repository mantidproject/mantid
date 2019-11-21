// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/OptionsDialog.h"
#include "MantidQtWidgets/Common/OptionsDialogModel.h"
#include "MantidQtWidgets/Common/OptionsDialogPresenter.h"
#include <QCloseEvent>
#include <QPushButton>

namespace MantidQt {
namespace MantidWidgets {

/** Constructor */
OptionsDialog::OptionsDialog(QWidget *parent) {
  Q_UNUSED(parent);
  initLayout();
  initBindings();
}

/** Destructor */
OptionsDialog::~OptionsDialog() {}

/** Initialise the ui */
void OptionsDialog::initLayout() {
  m_ui.setupUi(this);
  connect(m_ui.buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this,
          SLOT(notifySaveOptions()));
  connect(m_ui.buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this,
          SLOT(notifyLoadOptions()));
}

/** Bind options to their widgets */
void OptionsDialog::initBindings() {
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

/** This saves the currently configured options to the presenter */
void OptionsDialog::getOptions(std::map<std::string, bool> &boolOptions, std::map<std::string, int> &intOptions) {
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

/** This sets the ui to match the presenter's options */
void OptionsDialog::setOptions(std::map<std::string, bool> &boolOptions,
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

void OptionsDialog::subscribe(OptionsDialogSubscriber *notifyee) {
  m_notifyee = notifyee;
  notifyLoadOptions();
}

void OptionsDialog::notifyLoadOptions() { m_notifyee->loadOptions(); }
void OptionsDialog::notifySaveOptions() { m_notifyee->saveOptions(); }

void OptionsDialog::closeEvent(QCloseEvent *event) {
  notifyLoadOptions();
  QDialog::reject();

  // TODO investigate if event->ignore() is better
}

void OptionsDialog::show() { QDialog::exec(); }

} // namespace MantidWidgets
} // namespace MantidQt

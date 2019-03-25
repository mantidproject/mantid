// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/DataProcessorUI/QtDataProcessorOptionsDialog.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorPresenter.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorView.h"
#include <QPushButton>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
/** Constructor */
QtDataProcessorOptionsDialog::QtDataProcessorOptionsDialog(
    DataProcessorView *view, DataProcessorPresenter *presenter)
    : QDialog(dynamic_cast<QWidget *>(view)), m_presenter(presenter) {
  initLayout();
  initBindings();
  loadOptions();
}

/** Destructor */
QtDataProcessorOptionsDialog::~QtDataProcessorOptionsDialog() {}

/** Initialise the ui */
void QtDataProcessorOptionsDialog::initLayout() {
  ui.setupUi(this);
  connect(ui.buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this,
          SLOT(saveOptions()));
}

/** Bind options to their widgets */
void QtDataProcessorOptionsDialog::initBindings() {
  m_bindings.clear();

  // Check all the widgets for the "reflOptionName" property.
  // If it exists, bind the named option to that widget.
  QList<QWidget *> widgets = findChildren<QWidget *>();
  for (auto & widget : widgets) {
    QVariant binding = widget->property("reflOptionName");
    if (binding.isValid())
      m_bindings[binding.toString()] = widget->objectName();
  }
}

/** This slot saves the currently configured options to the presenter */
void QtDataProcessorOptionsDialog::saveOptions() {
  std::map<QString, QVariant> options = m_presenter->options();

  // Iterate through all our bound widgets, pushing their value into the options
  // map
  for (auto &binding : m_bindings) {
    QString widgetName = binding.second;
    if (widgetName.isEmpty())
      continue;

    QCheckBox *checkbox = findChild<QCheckBox *>(widgetName);
    if (checkbox) {
      options[binding.first] = checkbox->isChecked();
      continue;
    }

    QSpinBox *spinbox = findChild<QSpinBox *>(widgetName);
    if (spinbox) {
      options[binding.first] = spinbox->value();
      continue;
    }
  }

  // Update the presenter's options
  m_presenter->setOptions(options);
}

/** This slot sets the ui to match the presenter's options */
void QtDataProcessorOptionsDialog::loadOptions() {
  std::map<QString, QVariant> options = m_presenter->options();

  // Set the values from the options
  for (auto & option : options) {
    QString widgetName = m_bindings[option.first];
    if (widgetName.isEmpty())
      continue;

    QCheckBox *checkbox = findChild<QCheckBox *>(widgetName);
    if (checkbox) {
      checkbox->setChecked(option.second.toBool());
      continue;
    }

    QSpinBox *spinbox = findChild<QSpinBox *>(widgetName);
    if (spinbox) {
      spinbox->setValue(option.second.toInt());
      continue;
    }
  }
}

} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt

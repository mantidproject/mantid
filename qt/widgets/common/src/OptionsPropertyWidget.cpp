// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/OptionsPropertyWidget.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidKernel/ConfigService.h"

#include <QComboBox>
#include <QCompleter>
#include <QLabel>
#include <QLineEdit>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace MantidQt::API {
//----------------------------------------------------------------------------------------------
/** Destructor
 */
OptionsPropertyWidget::~OptionsPropertyWidget() = default;

//----------------------------------------------------------------------------------------------
/** Constructor
 */
OptionsPropertyWidget::OptionsPropertyWidget(Mantid::Kernel::Property *prop, QWidget *parent, QGridLayout *layout,
                                             int row)
    : PropertyWidget(prop, parent, layout, row) {
  // Label at column 0
  m_label = new QLabel(QString::fromStdString(prop->name()), m_parent);
  m_label->setToolTip(m_doc);
  m_gridLayout->addWidget(m_label, m_row, 0);
  m_widgets.push_back(m_label);

  // It is a choice of certain allowed values and can use a combination box
  // Check if this is the row that matches the one that we want to link to the
  // output box and used the saved combo box
  m_combo = new QComboBox(this);
  m_combo->setToolTip(m_doc);
  if (std::string(prop->type()).find("Workspace") != std::string::npos) {
    m_combo->setEditable(true);
    m_combo->setInsertPolicy(QComboBox::NoInsert);
    m_combo->completer()->setCompletionMode(QCompleter::PopupCompletion);
    connect(m_combo->lineEdit(), SIGNAL(editingFinished()), SLOT(editingFinished()));
  }
  m_combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
  m_combo->setMinimumContentsLength(20);
  m_widgets.push_back(m_combo);

  std::vector<std::string> items = prop->allowedValues();
  for (auto &item : items) {
    m_combo->addItem(QString::fromStdString(item));
  }
  // Make current value visible
  this->setValue(QString::fromStdString(m_prop->value()));

  // Make sure the connection comes after updating any values
  connect(m_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(userEditedProperty()));

  // Put the combo in column 1
  m_gridLayout->addWidget(m_combo, m_row, 1);
}

//----------------------------------------------------------------------------------------------
/** @return the value of the property, as typed in the GUI, as a string */
QString OptionsPropertyWidget::getValue() const { return m_combo->currentText(); }

//----------------------------------------------------------------------------------------------
/** Set the value into the GUI
 *
 * @param value :: string representation of the value */
void OptionsPropertyWidget::setValueImpl(const QString &value) {
  const QString temp = value.isEmpty() ? QString::fromStdString(m_prop->getDefault()) : value;

  int index = m_combo->findText(temp);
  if (index >= 0)
    m_combo->setCurrentIndex(index);
}

//----------------------------------------------------------------------------------------------
/** Performs validation of the inputs when editing is finished */
void OptionsPropertyWidget::editingFinished() {
  auto value = m_combo->currentText();
  const QString temp = value.isEmpty() ? QString::fromStdString(m_prop->getDefault()) : value;
  int index = m_combo->findText(temp);
  if (index >= 0)
    m_combo->setCurrentIndex(index);
  else {
    updateIconVisibility();
  }
}

} // namespace MantidQt::API

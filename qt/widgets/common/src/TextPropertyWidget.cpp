// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/TextPropertyWidget.h"

#include "MantidKernel/MaskedProperty.h"

using namespace Mantid::Kernel;

namespace MantidQt::API {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
TextPropertyWidget::TextPropertyWidget(Mantid::Kernel::Property *prop, QWidget *parent, QGridLayout *layout, int row)
    : PropertyWidget(prop, parent, layout, row) {
  // Label at column 0
  m_label = new QLabel(QString::fromStdString(prop->name()), m_parent);
  m_label->setToolTip(m_doc);
  setLabelFont(prop, m_label);
  m_gridLayout->addWidget(m_label, m_row, 0);
  m_widgets.push_back(m_label);

  // Text box at column 1
  m_textbox = new QLineEdit(m_parent);
  m_textbox->setToolTip(m_doc);
  setFieldPlaceholderText(prop, m_textbox);
  // Make current value visible
  this->setValue(QString::fromStdString(m_prop->value()));
  // Make sure the connection comes after updating any values
  connect(m_textbox, SIGNAL(editingFinished()), this, SLOT(userEditedProperty()));
  m_gridLayout->addWidget(m_textbox, m_row, 1);
  m_widgets.push_back(m_textbox);

  // Check if this is a masked property
  Mantid::Kernel::MaskedProperty<std::string> *maskedProp =
      dynamic_cast<Mantid::Kernel::MaskedProperty<std::string> *>(prop);
  // Make it echo those little stars
  if (maskedProp)
    m_textbox->setEchoMode(QLineEdit::Password);
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
TextPropertyWidget::~TextPropertyWidget() = default;

//----------------------------------------------------------------------------------------------
/** @return the value of the property, as typed in the GUI, as a string */
QString TextPropertyWidget::getValue() const { return m_textbox->text(); }

//----------------------------------------------------------------------------------------------
/** Set the value into the GUI
 *
 * @param value :: string representation of the value */
void TextPropertyWidget::setValueImpl(const QString &value) { m_textbox->setText(value); }

} // namespace MantidQt::API

#include "MantidQtWidgets/Common/TextPropertyWidget.h"
#include "MantidKernel/MaskedProperty.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;

namespace MantidQt {
namespace API {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
TextPropertyWidget::TextPropertyWidget(Mantid::Kernel::Property *prop,
                                       QWidget *parent, QGridLayout *layout,
                                       int row)
    : PropertyWidget(prop, parent, layout, row) {
  // Label at column 0
  m_label = new QLabel(QString::fromStdString(prop->name()), m_parent);
  m_label->setToolTip(m_doc);
  setLabelFont(prop, m_label);
  m_gridLayout->addWidget(m_label, m_row, 0, nullptr);
  m_widgets.push_back(m_label);

  // Text box at column 1
  m_textbox = new QLineEdit(m_parent);
  m_textbox->setToolTip(m_doc);
  setFieldPlaceholderText(prop, m_textbox);
  connect(m_textbox, SIGNAL(editingFinished()), this,
          SLOT(userEditedProperty()));
  m_gridLayout->addWidget(m_textbox, m_row, 1, nullptr);
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
TextPropertyWidget::~TextPropertyWidget() {}

//----------------------------------------------------------------------------------------------
/** @return the value of the property, as typed in the GUI, as a string */
QString TextPropertyWidget::getValue() const { return m_textbox->text(); }

//----------------------------------------------------------------------------------------------
/** Set the value into the GUI
 *
 * @param value :: string representation of the value */
void TextPropertyWidget::setValueImpl(const QString &value) {
  m_textbox->setText(value);
}

} // namespace API
} // namespace MantidQt

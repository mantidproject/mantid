#include "MantidQtAPI/TextPropertyWidget.h"
#include "MantidKernel/System.h"
#include "MantidKernel/MaskedProperty.h"

using namespace Mantid::Kernel;

namespace MantidQt
{
namespace API
{



  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  TextPropertyWidget::TextPropertyWidget(Mantid::Kernel::Property * prop, QWidget * parent, QGridLayout * layout, int row)
  : PropertyWidget(prop, parent, layout, row)
  {
    // Label at column 0
    m_label = new QLabel(QString::fromStdString(prop->name()), m_parent);
    m_label->setToolTip(m_doc);
    m_gridLayout->addWidget(m_label, m_row, 0, 0);
    m_widgets.push_back(m_label);

    // Text box at column 1
    m_textbox = new QLineEdit(m_parent);
    m_textbox->setToolTip(m_doc);
    connect(m_textbox, SIGNAL(editingFinished()), this, SLOT(valueChangedSlot()));
    m_gridLayout->addWidget(m_textbox, m_row, 1, 0);
    m_widgets.push_back(m_textbox);

    // Check if this is a masked property
    Mantid::Kernel::MaskedProperty<std::string> * maskedProp = dynamic_cast<Mantid::Kernel::MaskedProperty<std::string> *>(prop);
    // Make it echo those little stars
    if (maskedProp)
      m_textbox->setEchoMode(QLineEdit::Password);

  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  TextPropertyWidget::~TextPropertyWidget()
  {
  }
  
  //----------------------------------------------------------------------------------------------
  /** @return the value of the property, as typed in the GUI, as a string */
  QString TextPropertyWidget::getValue() const
  {
    return m_textbox->text();
  }

  //----------------------------------------------------------------------------------------------
  /** Set the value into the GUI
   *
   * @param value :: string representation of the value */
  void TextPropertyWidget::setValue(const QString & value)
  {
    QString temp = value;
    if (temp.isEmpty() && !m_prop->isDefault())
      temp = QString::fromStdString(m_prop->value());

    m_textbox->setText(temp);
  }

} // namespace MantidQt
} // namespace API

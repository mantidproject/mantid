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
    m_label = new QLabel(QString::fromStdString(prop->name()), this);
    m_label->setToolTip(m_doc);
    m_gridLayout->addWidget(m_label, m_row, 0, 0);

    // Text box at column 1
    m_textbox = new QLineEdit(this);
    m_textbox->setToolTip(m_doc);
    m_gridLayout->addWidget(m_textbox, m_row, 1, 0);

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
  


} // namespace MantidQt
} // namespace API

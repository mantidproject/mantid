#include "MantidQtAPI/TextPropertyWidget.h"
#include "MantidKernel/System.h"

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
    m_gridLayout->addWidget(m_label, m_row, 0, 0);

    // Text box at column 1
    m_textbox = new QLineEdit(this);
    m_gridLayout->addWidget(m_textbox, m_row, 1, 0);
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  TextPropertyWidget::~TextPropertyWidget()
  {
  }
  


} // namespace MantidQt
} // namespace API

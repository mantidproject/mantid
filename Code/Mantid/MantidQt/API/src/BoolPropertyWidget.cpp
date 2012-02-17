#include "MantidQtAPI/BoolPropertyWidget.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;

namespace MantidQt
{
namespace API
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  BoolPropertyWidget::BoolPropertyWidget(Mantid::Kernel::PropertyWithValue<bool> * prop, QWidget * parent, QGridLayout * layout, int row)
  : PropertyWidget(prop, parent, layout, row)
  {
    m_checkBox = new QCheckBox(QString::fromStdString(prop->name()), this);
    m_checkBox->setToolTip(m_doc);

    // Add the checkbox at column 1
    m_gridLayout->addWidget(m_checkBox, m_row, 1, 0);
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  BoolPropertyWidget::~BoolPropertyWidget()
  {
  }
  


} // namespace MantidQt
} // namespace API

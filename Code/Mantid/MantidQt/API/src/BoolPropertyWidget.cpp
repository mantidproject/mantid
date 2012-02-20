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
    m_checkBox = new QCheckBox(QString::fromStdString(prop->name()), m_parent);
    m_checkBox->setToolTip(m_doc);
    connect(m_checkBox, SIGNAL(stateChanged(int)), this, SLOT(valueChangedSlot()));
    m_widgets.push_back(m_checkBox);

    // Add the checkbox at column 1
    m_gridLayout->addWidget(m_checkBox, m_row, 1, 0);
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  BoolPropertyWidget::~BoolPropertyWidget()
  {
  }
  


  //----------------------------------------------------------------------------------------------
  /** @return the value of the property, as typed in the GUI, as a string */
  QString BoolPropertyWidget::getValue() const
  {
    if (m_checkBox->isChecked())
      return "1";
    else
      return "0";
  }

  //----------------------------------------------------------------------------------------------
  /** Set the value into the GUI
   *
   * @param value :: string representation of the value */
  void BoolPropertyWidget::setValue(const QString & value)
  {
    QString temp = value;
    if (temp.isEmpty())
      temp = QString::fromStdString(m_prop->value());

    if (temp == "0")
      m_checkBox->setCheckState(Qt::Unchecked);
    else
      m_checkBox->setCheckState(Qt::Checked);
  }
} // namespace MantidQt
} // namespace API

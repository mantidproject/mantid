#include "MantidQtAPI/OptionsPropertyWidget.h"
#include "MantidKernel/System.h"
#include <QtGui>
#include <QLabel>
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidKernel/ConfigService.h"
#include <qcombobox.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace MantidQt
{
namespace API
{

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  OptionsPropertyWidget::~OptionsPropertyWidget()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  OptionsPropertyWidget::OptionsPropertyWidget(Mantid::Kernel::Property * prop, QWidget * parent, QGridLayout * layout, int row)
  : PropertyWidget(prop, parent, layout, row)
  {
    // Label at column 0
    m_label = new QLabel(QString::fromStdString(prop->name()), m_parent);
    m_label->setToolTip(m_doc);
    m_gridLayout->addWidget(m_label, m_row, 0, 0);
    m_widgets.push_back(m_label);

    //It is a choice of certain allowed values and can use a combination box
    //Check if this is the row that matches the one that we want to link to the
    //output box and used the saved combo box
    m_combo = new QComboBox(this);
    m_combo->setToolTip(m_doc);
    m_widgets.push_back(m_combo);

    std::vector<std::string> items = prop->allowedValues();
    for(auto vitr = items.begin(); vitr != items.end(); ++vitr)
    {
      m_combo->addItem(QString::fromStdString(*vitr));
    }
    // Make current value visible
    this->setValue(QString::fromStdString(m_prop->value()));

    // Make sure the connection comes after updating any values
    connect(m_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(userEditedProperty()));

    // Put the combo in column 1
    m_gridLayout->addWidget(m_combo, m_row, 1, 0);
  }

  //----------------------------------------------------------------------------------------------
  /** @return the value of the property, as typed in the GUI, as a string */
  QString OptionsPropertyWidget::getValue() const
  {
    return m_combo->currentText();
  }

  //----------------------------------------------------------------------------------------------
  /** Set the value into the GUI
   *
   * @param value :: string representation of the value */
  void OptionsPropertyWidget::setValueImpl(const QString & value)
  {
    const QString temp = value.isEmpty() ? QString::fromStdString(m_prop->getDefault()) : value;

    int index = m_combo->findText(temp);
    if( index >= 0 )
      m_combo->setCurrentIndex(index);
  }
} // namespace MantidQt
} // namespace API

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

    // Check whether we should display hidden workspaces in WS comboboxes
    QString setting = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("MantidOptions.InvisibleWorkspaces"));
    bool showHidden = (!setting.compare("0")) ? false : true;

    //It is a choice of certain allowed values and can use a combination box
    //Check if this is the row that matches the one that we want to link to the
    //output box and used the saved combo box
    m_combo = new QComboBox(this);
    m_combo->setToolTip(m_doc);
    connect(m_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(valueChangedSlot()));
    m_widgets.push_back(m_combo);

    bool isWorkspaceProp(dynamic_cast<IWorkspaceProperty*>(prop));
    std::set<std::string> items = prop->allowedValues();
    std::set<std::string>::const_iterator vend = items.end();
    for(std::set<std::string>::const_iterator vitr = items.begin(); vitr != vend; ++vitr)
    {
      QString propValue = QString::fromStdString(*vitr);
      // Skip hidden workspaces
      if ( isWorkspaceProp && ( ! showHidden ) && propValue.startsWith("__") ) continue;
      m_combo->addItem(propValue);
    }

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
  void OptionsPropertyWidget::setValue(const QString & value)
  {
    QString temp = value;
    if (temp.isEmpty())
      temp = QString::fromStdString(m_prop->value());

    int index = m_combo->findText(temp);
    if( index >= 0 )
      m_combo->setCurrentIndex(index);
  }
} // namespace MantidQt
} // namespace API

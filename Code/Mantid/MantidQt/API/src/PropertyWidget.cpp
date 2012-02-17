#include "MantidQtAPI/PropertyWidget.h"
#include "MantidKernel/System.h"
#include "MantidAPI/IWorkspaceProperty.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::API::IWorkspaceProperty;

namespace MantidQt
{
namespace API
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  PropertyWidget::PropertyWidget(Mantid::Kernel::Property * prop, QWidget * parent, QGridLayout * layout, int row)
  : QWidget(parent),
    m_prop(prop), m_gridLayout(layout), m_row(row),
    m_replaceWSButton(NULL)
  {
    if (!prop)
      throw std::runtime_error("NULL Property passed to the PropertyWidget constructor.");

    if (!m_gridLayout)
    {
      // Create a LOCAL grid layout
      m_gridLayout = new QGridLayout(this);
      this->setLayout(m_gridLayout);
      // Will always go to row 0
      m_row = 0;
//      this->setStyleSheet(          "QWidget { border: 1px solid gray;  }"          );
    }

    // Create the validator label (that red star)
    m_validLbl = new QLabel("*");
    QPalette pal = m_validLbl->palette();
    pal.setColor(QPalette::WindowText, Qt::darkRed);
    m_validLbl->setPalette(pal);
    // Start off hidden
    m_validLbl->setVisible(false);
    // Put it in the 4th column.
    m_gridLayout->addWidget(m_validLbl, m_row, 4);

    /// Save the documentation tooltip
    m_doc = QString::fromStdString(prop->documentation());

    IWorkspaceProperty * wsProp = dynamic_cast<IWorkspaceProperty*>(m_prop);
    if (wsProp && (m_prop->direction() == Direction::Output) )
    {
      m_replaceWSButton = new QPushButton(QIcon(":/data_replace.png"), "");
      // MG: There is no way with the QIcon class to actually ask what size it is so I had to hard
      // code this number here to get it to a sensible size
      m_replaceWSButton->setMaximumWidth(32);
      //m_wsbtn_tracker[btn ] = 1;
      m_replaceWSButton->setToolTip("Replace input workspace");
      connect(m_replaceWSButton, SIGNAL(clicked()), this, SLOT(replaceWSButtonClicked()));
    }
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  PropertyWidget::~PropertyWidget()
  {
  }
  
  //----------------------------------------------------------------------------------------------
  /** Returns true if the widget is being added to an existing
   * GridLayout.
   *
   * @return true/false
   */
  bool PropertyWidget::inGrid() const
  {
    return bool(m_gridLayout);
  }


  /** Slot called when someone clicks the "replace ws button" */
  void PropertyWidget::replaceWSButtonClicked()
  {
  }


} // namespace MantidQt
} // namespace API

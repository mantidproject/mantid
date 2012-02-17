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

//    // Create the validator label (that red star)
//    m_validLbl = new QLabel("*");
//    QPalette pal = m_validLbl->palette();
//    pal.setColor(QPalette::WindowText, Qt::darkRed);
//    m_validLbl->setPalette(pal);
//    // Start off hidden
//    m_validLbl->setVisible(false);
//    // Put it in the 4th column.
//    m_gridLayout->addWidget(m_validLbl, m_row, 4);

    /// Save the documentation tooltip
    m_doc = QString::fromStdString(prop->documentation());

    IWorkspaceProperty * wsProp = dynamic_cast<IWorkspaceProperty*>(m_prop);
    if (wsProp && (m_prop->direction() == Direction::Output) )
    {
      m_replaceWSButton = new QPushButton(QIcon(":/data_replace.png"), "", this);
      // MG: There is no way with the QIcon class to actually ask what size it is so I had to hard
      // code this number here to get it to a sensible size
      m_replaceWSButton->setMaximumWidth(32);
      //m_wsbtn_tracker[btn ] = 1;
      m_replaceWSButton->setToolTip("Replace input workspace");
      connect(m_replaceWSButton, SIGNAL(clicked()), this, SLOT(replaceWSButtonClicked()));
      m_widgets.push_back(m_replaceWSButton);
      // Place in the grid on column 2.
      m_gridLayout->addWidget(m_replaceWSButton, m_row, 2);
    }
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  PropertyWidget::~PropertyWidget()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Slot called when someone clicks the "replace ws button" */
  void PropertyWidget::replaceWSButtonClicked()
  {
    emit replaceWorkspaceName(QString::fromStdString(m_prop->name()));
  }

  //----------------------------------------------------------------------------------------------
  /** Show or hide the "Replace WS" button
   *
   * @param show :: true to show it */
  void PropertyWidget::showReplaceWSButton(bool show)
  {
    if (m_replaceWSButton)
    {
      m_replaceWSButton->setVisible(show);
      //std::cout << "m_replaceWSButton is " << show << std::endl;
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Slot called when the value of the property had been changed.
   * Called by all sub-classes.
   * */
  void PropertyWidget::valueChangedSlot()
  {
    // This will be caught by the GenericDialog.
    emit valueChanged( QString::fromStdString(m_prop->name()) ) ;
  }

  //----------------------------------------------------------------------------------------------
  /** Sets all widgets contained within to Enabled
   * @param val :: enabled or not   */
  void PropertyWidget::setEnabled(bool val)
  {
    for (int i=0; i < m_widgets.size(); i++)
      m_widgets[i]->setEnabled(val);
  }

  //----------------------------------------------------------------------------------------------
  /** Sets all widgets contained within to Visible
   * @param val :: Visible or not   */
  void PropertyWidget::setVisible(bool val)
  {
    for (int i=0; i < m_widgets.size(); i++)
      m_widgets[i]->setVisible(val);
  }


} // namespace MantidQt
} // namespace API

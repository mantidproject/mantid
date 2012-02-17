#include "MantidQtAPI/PropertyWidget.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;

namespace MantidQt
{
namespace API
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  PropertyWidget::PropertyWidget(Mantid::Kernel::Property * prop, QWidget * parent, QGridLayout * layout, int row)
  : QWidget(parent),
    m_prop(prop), m_gridLayout(layout), m_row(row)
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
    // Put it in the 4th column.
    m_gridLayout->addWidget(m_validLbl, m_row, 4);
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


} // namespace MantidQt
} // namespace API

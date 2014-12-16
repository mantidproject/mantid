//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/DataComparison.h"

//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
namespace CustomInterfaces
{
  DECLARE_SUBWINDOW(DataComparison);
}
}

using namespace MantidQt::CustomInterfaces;

//----------------------
// Public member functions
//----------------------
///Constructor
DataComparison::DataComparison(QWidget *parent) :
  UserSubWindow(parent),
  m_plot(new QwtPlot(parent))
{
}

/// Set up the dialog layout
void DataComparison::initLayout()
{
  m_uiForm.setupUi(this);

  // Add the plot to the UI
  m_plot->setCanvasBackground(Qt::white);
  m_uiForm.loPlot->addWidget(m_plot);

  // Connect push buttons
  connect(m_uiForm.pbAddData, SIGNAL(clicked()), this, SLOT(addData()));

  connect(m_uiForm.pbRemoveSelectedData, SIGNAL(clicked()), this, SLOT(removeSelectedData()));
  connect(m_uiForm.pbRemoveAllData, SIGNAL(clicked()), this, SLOT(removeAllData()));

  connect(m_uiForm.pbDiffSelected, SIGNAL(clicked()), this, SLOT(diffSelected()));
  connect(m_uiForm.pbClearDiff, SIGNAL(clicked()), this, SLOT(clearDiff()));
}

/**
 * Adds the data currently selected by the data selector to the plot.
 */
void DataComparison::addData()
{
}

/**
 * Removes the data currently selected in the table from the plot.
 */
void DataComparison::removeSelectedData()
{
}

/**
 * Removed all loaded data from the plot.
 */
void DataComparison::removeAllData()
{
}

/**
 * Creates a diff workspace of the two currently selected workspaces in the table
 * and plots it on the plot.
 *
 * Does nothing if there are not 2 workspaces selected.
 */
void DataComparison::diffSelected()
{
}

/**
 * Removes the diff workspace form the plot.
 *
 * Does not remove it from ADS.
 */
void DataComparison::clearDiff()
{
}

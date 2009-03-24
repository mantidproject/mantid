//-----------------------------
// Includes
//-----------------------------
#include "MantidQtCustomInterfaces/SANSUtilityDialogs.h"

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QInputDialog>
#include <QMessageBox>
#include <QShortcut>

using namespace MantidQt::CustomInterfaces;

//-----------------------------
// Public member functions
//-----------------------------
/**
 * Default constructor
 */
SANSPlotDialog::SANSPlotDialog(QWidget *parent) : 
  QDialog(parent), m_workspaces()
{
  setWindowTitle("SANS - Plot Dialog");

  m_opt_input = new QTreeWidget;
  m_opt_input->setColumnCount(2);
  QStringList headers("Name");
  headers << "Spectra";
  m_opt_input->setHeaderLabels(headers);
    
  QHBoxLayout *top_layout = new QHBoxLayout;
  top_layout->addWidget(m_opt_input);

  QGridLayout *grid = new QGridLayout;
  grid->addWidget(new QLabel("Data Set"), 0, 0);
  m_data_sets = new QComboBox;
  grid->addWidget(m_data_sets, 0, 1);
  
  grid->addWidget(new QLabel("Spectra"), 1, 0);
  m_spec_list = new QLineEdit;
  grid->addWidget(m_spec_list, 1, 1);

  grid->addWidget(new QLabel("Plot"), 2, 0);
  m_plots = new QComboBox;
  m_plots->addItem("New Plot ...");
  m_plots->addItem("Plot 1");
  m_plots->setCurrentIndex(1);
  connect(m_plots, SIGNAL(activated(const QString &)), this, SLOT(plotOptionClicked(const QString&)));
  grid->addWidget(m_plots, 2, 1);
  top_layout->addLayout(grid);

  QPushButton *addToList = new QPushButton("Add to plot");
  connect(addToList, SIGNAL(clicked()), this, SLOT(addNewPlot()));
  grid->addWidget(addToList, 3, 0, 1, 2, Qt::AlignHCenter);

  QVBoxLayout *main_layout = new QVBoxLayout;
  main_layout->addLayout(top_layout);
  QPushButton *plot = new QPushButton("Plot", this);
  plot->setDefault(true);
  connect(plot, SIGNAL(clicked()), this, SLOT(plotButtonClicked()));
  QPushButton *close = new QPushButton("Close", this);
  connect(close, SIGNAL(clicked()), this, SLOT(close()));  

  QHBoxLayout *bottom = new QHBoxLayout;
  bottom->addStretch();
  bottom->addWidget(plot);
  bottom->addWidget(close);

  main_layout->addLayout(bottom);
  setLayout(main_layout);

  QShortcut *delete_key = new QShortcut(QKeySequence(Qt::Key_Delete), this);
  connect(delete_key, SIGNAL(activated()), this, SLOT(deleteKeyPressed()));
}

/** 
 * Set the list of data sets that are available to plot
 * @param workspaces A list of string indicating the available data
 */
void SANSPlotDialog::setAvailableData(const QStringList & workspaces)
{
  m_workspaces = workspaces;
  m_data_sets->clear();
  m_data_sets->addItems(workspaces);
}


/**
 * Add a new plot to the list
 */
void SANSPlotDialog::addNewPlot()
{
  if( m_data_sets->count() == 0 )
  {
    QMessageBox::information(this, "New Plot","There is no data available to plot");
    return;
  }

  if( m_spec_list->text().isEmpty() )
  {
    QMessageBox::information(this, "New Plot","No spectra numbers have been entered");
    return;
  }
  //Check that a valid option has been selected
  plotOptionClicked(m_plots->currentText());

  QString name = m_plots->currentText();
  QList<QTreeWidgetItem *> searchlist = m_opt_input->findItems(name, Qt::MatchExactly);
  if( searchlist.isEmpty() )
  {
    QTreeWidgetItem *topitem = new QTreeWidgetItem(m_opt_input, QStringList(name));
    //Add new set as a child
    QTreeWidgetItem *dataset = new QTreeWidgetItem(topitem);
    dataset->setText(0, m_data_sets->currentText());
    dataset->setText(1, m_spec_list->text());
  }
  else 
  {
    QTreeWidgetItem *topitem = searchlist[0];
    int child_count = topitem->childCount();
    QTreeWidgetItem *dataset = NULL;
    for( int index = 0; index < child_count; ++index )
    {
      if( topitem->child(index)->text(0) == m_data_sets->currentText() )
      {
	dataset = topitem->child(index);
	break;
      }
    }
    if( dataset )
    {
      dataset->setText(1, dataset->text(1) + "," + m_spec_list->text());
    }
    else
    {
      QTreeWidgetItem *dataset = new QTreeWidgetItem(topitem);
      dataset->setText(0, m_data_sets->currentText());
      dataset->setText(1, m_spec_list->text());
    }
  }
  m_spec_list->clear();
  m_opt_input->expandAll();
}

/**
 * Construct the python code to plot the graphs
 */
void SANSPlotDialog::plotButtonClicked()
{
  QString py_code;
  QTreeWidgetItem *root = m_opt_input->invisibleRootItem();
  int toplevel_count = root->childCount();
  for( int p = 0; p < toplevel_count; ++p )
  {
    //top-level item, i.e. plot name
    QTreeWidgetItem *top_item = root->child(p);
    py_code += QString("plot") + QString::number(p) + "= ";
    int no_child = top_item->childCount();
    bool first_curve(true);
    for( int c = 0; c < no_child; ++c )
    {
      QTreeWidgetItem *item = top_item->child(c);
      QString data = item->text(0);
      QStringList spectra = item->text(1).split(',', QString::SkipEmptyParts);
      QStringListIterator itr(spectra);
      //The very first curve is special as it creates the referece to the plot that is needed to add the others
      if( first_curve )
      {
	py_code += writePlotCmd(data, itr.next(), true) + "\n";
	first_curve = false;
      }
      while( itr.hasNext() )
      {
	py_code += "plot" + QString::number(p) + ".insertCurve(" + writePlotCmd(data, itr.next(), false) + ", 0)\n";
      }
    }
  }
  QMessageBox::information(this, "", py_code);
  if( !py_code.isEmpty() ) 
  {
    emit pythonCodeConstructed(py_code);
  }
  m_opt_input->clear();
}

/**
 * User clicked for on plot name
 */
void SANSPlotDialog::plotOptionClicked(const QString & item_text)
{
  if( !item_text.endsWith("...") ) return;

  int next_num = m_plots->count();

  m_plots->addItem("Plot " + QString::number(next_num));
  m_plots->setCurrentIndex(m_plots->count() - 1);
}

/**
 * Write a Python plot command
 * @param workspace The workspace name
 * @param spec_num The spectrum number
 * @param show_plot Whether to make the plot visibl or not
 */
QString SANSPlotDialog::writePlotCmd(const QString & workspace, const QString & spec_num, bool show_plot)
{
  QString py_code = "plotSpectrum('" + workspace + "', " + spec_num;
  if( !show_plot )
  {
    py_code += ", False";
  }
  py_code += ")";
  return py_code;
}

/**
 * Respond to the delete key being pressed
 */
void SANSPlotDialog::deleteKeyPressed()
{
  QList<QTreeWidgetItem *> items = m_opt_input->selectedItems();
  QListIterator<QTreeWidgetItem *> itr(items);
  while( itr.hasNext() )
  {
    delete itr.next();
  }
}

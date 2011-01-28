//-----------------------------
// Includes
//-----------------------------
#include "MantidQtCustomInterfaces/SANSUtilityDialogs.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"

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

namespace MantidQt
{
namespace CustomInterfaces
{


//-----------------------------
// Public member functions
//-----------------------------
/**
 * Default constructor
 */
SANSPlotDialog::SANSPlotDialog(QWidget *parent) : 
  API::MantidDialog(parent), m_workspaces()
{
  setWindowTitle("SANS - Plot Dialog");

  m_opt_input = new QTreeWidget;
  m_opt_input->setColumnCount(2);
  QStringList headers("Name");
  headers << "Details";
  m_opt_input->setHeaderLabels(headers);
    
  QHBoxLayout *top_layout = new QHBoxLayout;
  top_layout->addWidget(m_opt_input);

  QGridLayout *grid = new QGridLayout;
  grid->addWidget(new QLabel("Data Set"), 0, 0);
  m_data_sets = new QComboBox;
  grid->addWidget(m_data_sets, 0, 1);
  

  grid->addWidget(new QLabel("Plot"), 1, 0);
  m_plots = new QComboBox;
  m_plots->addItem("New Plot ...");
  m_plots->addItem("Plot 1");
  m_plots->setCurrentIndex(1);
  connect(m_plots, SIGNAL(activated(const QString &)), this, SLOT(plotOptionClicked(const QString&)));
  grid->addWidget(m_plots, 1, 1);
  top_layout->addLayout(grid);

  QPushButton *add1D = new QPushButton("Add 1D");
  connect(add1D, SIGNAL(clicked()), this, SLOT(add1DPlot()));
  m_spec_list = new QLineEdit;
  m_spec_list->setText("1");
  m_spec_list->setToolTip("A comma-separated list of workspace indexes");
  grid->addWidget(add1D, 2,0);
  grid->addWidget(m_spec_list, 2, 1);

  QPushButton *add2D = new QPushButton("Add 2D");
  connect(add2D, SIGNAL(clicked()), this, SLOT(add2DPlot()));
  grid->addWidget(add2D, 3, 0, Qt::AlignHCenter);
  
  QVBoxLayout *main_layout = new QVBoxLayout;
  main_layout->addLayout(top_layout);
  QPushButton *plot = new QPushButton("Plot", this);
  plot->setDefault(true);
  connect(plot, SIGNAL(clicked()), this, SLOT(plotButtonClicked()));
  QPushButton *close = new QPushButton("Close", this);
  connect(close, SIGNAL(clicked()), this, SLOT(close()));  

  QHBoxLayout *bottom = new QHBoxLayout;
  m_info_lbl = new QLabel("");
  bottom->addWidget(m_info_lbl);
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
 * @param workspaces :: A list of string indicating the available data
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
void SANSPlotDialog::add1DPlot()
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
  QString ws = m_data_sets->currentText();
  if( searchlist.isEmpty() )
  {
    //Check spectra before any items are added
    QString spec_nums = checkSpectraList(ws, m_spec_list->text());
    if( spec_nums.isEmpty() ) return;
    QTreeWidgetItem *topitem = new QTreeWidgetItem(m_opt_input, QStringList(name));
    //Add new set as a child
    QTreeWidgetItem *dataset = new QTreeWidgetItem(topitem);
    dataset->setText(0, ws);
    dataset->setText(1, spec_nums);
  }
  else 
  {
    QTreeWidgetItem *topitem = searchlist[0];
    int child_count = topitem->childCount();
    QTreeWidgetItem *dataset = NULL;
    for( int index = 0; index < child_count; ++index )
    {
      if( topitem->child(index)->text(0) == ws )
      {
	dataset = topitem->child(index);
	break;
      }
    }
    if( dataset )
    {
      QString allnums = dataset->text(1) + "," +  m_spec_list->text();
      QString spec_nums = checkSpectraList(ws, allnums);
      if( spec_nums.isEmpty() ) return;
      dataset->setText(1, spec_nums);
    }
    else
    {
      QTreeWidgetItem *dataset = new QTreeWidgetItem(topitem);
      dataset->setText(0, ws);
      QString spec_nums = checkSpectraList(ws, m_spec_list->text());
      if( spec_nums.isEmpty() ) return;
      dataset->setText(1, spec_nums);
    }
  }
  m_spec_list->setText("1");
  m_opt_input->expandAll();
}

/**
 * Add a 2D plot to the list
 */
void SANSPlotDialog::add2DPlot()
{
  QString ws = m_data_sets->currentText();
  QTreeWidgetItem *plot2D =  new QTreeWidgetItem(QStringList(ws));
  plot2D->setData(0, Qt::UserRole, 100);
  plot2D->setText(1, "Color map plot");
  m_opt_input->addTopLevelItem(plot2D);
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
    //Check for 2D plot
    if( top_item->data(0, Qt::UserRole).toInt() == 100 )
    {
      py_code += "m = importMatrixWorkspace('" + top_item->text(0) + "')\n"
	"m.plotGraph2D()\n"
	"m.hide()\n";
    }
    else
    {
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
	  int spec = itr.next().toInt() - 1;
	  py_code += writePlotCmd(data, QString::number(spec), true) + "\n";
	  first_curve = false;
	}
	while( itr.hasNext() )
	{
	  int spec = itr.next().toInt() - 1;
	  QString plotcmd = writePlotCmd(data, QString::number(spec), false);
	  if( !plotcmd.isEmpty() )
	  {
	    py_code += "plot" + QString::number(p) + ".insertCurve(" + plotcmd + ", 0)\n";
	  }
	}
      }
    }
  }
  if( !py_code.isEmpty() ) 
  {
    emit pythonCodeConstructed(py_code);
  }
  m_opt_input->clear();
  m_opt_input->reset();
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
 * Write a Python plot command (this assumes the input has already been checked)
 * @param workspace :: The workspace name
 * @param spec_num :: The spectrum number
 * @param show_plot :: Whether to make the plot visibl or not
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
 * Check the current spectra list
 * @param workspace :: The workspace to check
 * @param speclist :: The list of spectra to check
 * @returns A valid list of spectra from the box
 */
QString SANSPlotDialog::checkSpectraList(const QString & workspace, const QString & speclist )
{
  // Check that the spectrum is within a valid range
  Mantid::API::MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
    (Mantid::API::AnalysisDataService::Instance().retrieve(workspace.toStdString()));
  if( ws == boost::shared_ptr<Mantid::API::MatrixWorkspace>() ) return QString("");

  int nhist = ws->getNumberHistograms();
  QStringList spectra = speclist.split(',', QString::SkipEmptyParts);
  QStringListIterator itr(spectra);
  QString validlist("");
  bool allvalid(true);
  while( itr.hasNext() )
  {
    QString entry = itr.next();
    if( validlist.contains(entry) ) continue;
    int spec = entry.toInt() - 1;
    if( spec < 0 || spec >= nhist ) 
    {
      allvalid = false;
      continue;
    }
    validlist += entry + ",";
  }
  if( allvalid ) m_info_lbl->setText("");
  else m_info_lbl->setText("An invalid spectra number was given");

  if( validlist.endsWith(',') ) validlist.truncate(validlist.count() - 1);
  return validlist;
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


} // namespace MantidQt

} // namespace CustomInterfaces

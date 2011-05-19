//---------------------------------------
// Includes
//---------------------------------------

#include "SequentialFitDialog.h"
#include "FitPropertyBrowser.h"
#include "MantidUI.h"
#include "SelectWorkspacesDialog.h"
#include "../ApplicationWindow.h"

#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/CompositeFunction.h"

#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>

//---------------------------------------
// Public member functions
//---------------------------------------

/// Constructor
SequentialFitDialog::SequentialFitDialog(FitPropertyBrowser* fitBrowser) :
QDialog(fitBrowser->m_appWindow),m_fitBrowser(fitBrowser)
{
  ui.setupUi(this);

  connect(ui.btnAddFile,SIGNAL(clicked()),this,SLOT(addFile()));
  connect(ui.btnAddWorkspace,SIGNAL(clicked()),this,SLOT(addWorkspace()));
  connect(ui.btnDelete,SIGNAL(clicked()),this,SLOT(removeItem()));

  connect(ui.btnFit,SIGNAL(clicked()),this,SLOT(accept()));
  connect(ui.btnCancel,SIGNAL(clicked()),this,SLOT(reject()));
  connect(ui.btnHelp,SIGNAL(clicked()),this,SLOT(helpClicked()));
  connect(ui.ckbLogPlot,SIGNAL(toggled(bool)),this,SLOT(plotAgainstLog(bool)));

  ui.cbLogValue->setEditable(true);
  ui.ckbLogPlot->setChecked(true);
  ui.sbPeriod->setValue(1);

  populateParameters();

  connect(fitBrowser,SIGNAL(functionChanged()),this,SLOT(functionChanged()));
  connect(this,SIGNAL(needShowPlot()),this,SLOT(showPlot()));
  connect(ui.tWorkspaces,SIGNAL(cellChanged(int,int)),this,SLOT(spectraChanged(int,int)));

}

void SequentialFitDialog::addWorkspace()
{
  SelectWorkspacesDialog* dlg = new SelectWorkspacesDialog(m_fitBrowser->m_appWindow);
  if (dlg->exec() == QDialog::Accepted)
  {
    addWorkspaces(dlg->getSelectedNames());
  }
}

bool SequentialFitDialog::addWorkspaces(const QStringList wsNames)
{
  if (wsNames.isEmpty()) return false;
  int row = ui.tWorkspaces->rowCount();
  ui.tWorkspaces->model()->insertRows(row,wsNames.size());
  int wi = m_fitBrowser->workspaceIndex();
  QAbstractItemModel* model = ui.tWorkspaces->model();
  foreach(QString name,wsNames)
  {
    model->setData(model->index(row,0,QModelIndex()),name);

    if (row == 0)
    {
      ui.ckbLogPlot->setChecked(validateLogs(name));
    }
      
    // disable the period cell
    model->setData(model->index(row,1,QModelIndex()),"");
    QTableWidgetItem* item = ui.tWorkspaces->item(row,1);
    if (item) 
    {
      item->setBackgroundColor(QColor(Qt::lightGray));
      item->setFlags(Qt::NoItemFlags);
    }

    if (ui.ckbLogPlot->isChecked())
    {
      // set spectrum number corresponding to the workspace index
      Mantid::API::MatrixWorkspace_sptr ws = 
        boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve(name.toStdString())
        );
      int spec = -1;
      if (ws)
      {
        Mantid::API::Axis* y = ws->getAxis(1);
        if (y->isSpectra())
        {
          spec = y->spectraNo(wi);
        }
      }
      //model->setData(model->index(row,2,QModelIndex()),spec);
      setSpectrum(row,spec);
      if (row == 0)
      {
        ui.sbSpectrum->setValue(spec);
      }

      // set workspace index
      //model->setData(model->index(row,3,QModelIndex()),wi);
      setWSIndex(row,wi);
    }
    ++row;
  }
  ui.tWorkspaces->resizeRowsToContents();
  ui.tWorkspaces->resizeColumnsToContents();
  return true;
}

void SequentialFitDialog::addFile()
{
  QFileDialog dlg(this);
  dlg.setFileMode(QFileDialog::ExistingFiles);
  const std::vector<std::string>& searchDirs =
    Mantid::Kernel::ConfigService::Instance().getDataSearchDirs();
  QString dir;
  if ( searchDirs.size() == 0 )
  {
    dir = "";
  }
  else
  {
    dir = QString::fromStdString(searchDirs.front());
  }
  dlg.setDirectory(dir);
  if (dlg.exec())
  {
    QStringList fileNames;
    fileNames = dlg.selectedFiles();
    if (fileNames.isEmpty()) return;
    fileNames.sort();

    int row = ui.tWorkspaces->rowCount();
    ui.tWorkspaces->model()->insertRows(row,fileNames.size());
    //int wi = m_fitBrowser->workspaceIndex();
    QAbstractItemModel* model = ui.tWorkspaces->model();
    foreach(QString name,fileNames)
    {
      model->setData(model->index(row,0,QModelIndex()),name); // file name
      model->setData(model->index(row,1,QModelIndex()),ui.sbPeriod->value());   // period
      model->setData(model->index(row,2,QModelIndex()),ui.sbSpectrum->value());   // spectrum
      model->setData(model->index(row,3,QModelIndex()),"");   // ws index
      QTableWidgetItem* item = ui.tWorkspaces->item(row,3);
      if (item) 
      {
        item->setBackgroundColor(QColor(Qt::lightGray));
        item->setFlags(Qt::NoItemFlags);
      }
      ++row;
    }
    ui.tWorkspaces->resizeRowsToContents();
    ui.tWorkspaces->resizeColumnsToContents();
  }
}

void SequentialFitDialog::removeItem()
{
  QList<QTableWidgetSelectionRange> ranges = ui.tWorkspaces->selectedRanges();
  while (!ranges.empty())
  {
    ui.tWorkspaces->model()->removeRows(ranges[0].topRow(),ranges[0].rowCount());
    ranges = ui.tWorkspaces->selectedRanges();
  }
}

bool SequentialFitDialog::validateLogs(const QString wsName)
{
  Mantid::API::MatrixWorkspace_sptr ws = 
    boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
    Mantid::API::AnalysisDataService::Instance().retrieve(wsName.toStdString())
    );
  if (ws)
  {
    const std::vector<Mantid::Kernel::Property*> logs = ws->run().getLogData();
    QStringList logNames;
    for(int i=0;i<static_cast<int>(logs.size());++i)
    {
      Mantid::Kernel::TimeSeriesProperty<double>* p = 
        dynamic_cast<Mantid::Kernel::TimeSeriesProperty<double>*>(logs[i]);
      if (!p) continue;
      logNames << QString::fromStdString(logs[i]->name());
    }
    int n = ui.cbLogValue->count();
    // if the ws has no logs - do not include it
    if (logNames.empty())
    {
      return false;
    }
    // if the log value combo box is empty fill it in with the log names from ws
    if (n == 0)
    {
      ui.cbLogValue->insertItems(0,logNames);
    }
    else
    {// keep only those logs which are included in both ui.cbLogValue and logNames
      QStringList namesToRemove;
      for(int i=0;i<n;++i)
      {
        QString name = ui.cbLogValue->itemText(i);
        if (!logNames.contains(name))
        {
          namesToRemove << name;
        }
      }
      foreach(QString name,namesToRemove)
      {
        int i = ui.cbLogValue->findText(name);
        if (i >= 0)
        {
          ui.cbLogValue->removeItem(i);
        }
      }
      if (ui.cbLogValue->count() == 0)
      {
        QMessageBox::warning(m_fitBrowser->m_appWindow,"MantidPlot - Warning","The list of the log names is empty:\n"
          "The selected workspaces do not have common logs");
        return false;
      }
    }
  }
  return true;
}

void SequentialFitDialog::accept()
{
  QStringList inputStr;
  for(int i=0;i<ui.tWorkspaces->rowCount();++i)
  {
    QString name = ui.tWorkspaces->model()->data(ui.tWorkspaces->model()->index(i,0)).toString();
    // check name whether it's a file or workspace in order to decide which number to use:
    // spectrum (for files) or workspace index (for workspaces)
    int j = 3;
    QString prefx;
    QTableWidgetItem* item = ui.tWorkspaces->item(i,3);
    bool isFile = !item || item->flags().testFlag(Qt::ItemIsEnabled) == false;
    if (ui.ckbLogPlot->isChecked())
    {// plot against logs
      if (isFile)
      {
        j = 2;
        prefx = "sp";
      }
      else
      {
        prefx = "i";
      }
    }
    else
    {// plot against axis values
      prefx = "v";
      j = 2;
    }
    QString index = ui.tWorkspaces->model()->data(ui.tWorkspaces->model()->index(i,j)).toString();
    QString parStr = name + "," + prefx + index;
    if (isFile)
    {// add the period
      parStr += QString(",") + ui.tWorkspaces->model()->data(ui.tWorkspaces->model()->index(i,1)).toString();
    }
    inputStr << parStr;
  }
  std::string funStr;
  if (m_fitBrowser->m_compositeFunction->nFunctions() > 1)
  {
    funStr = *m_fitBrowser->m_compositeFunction;
  }
  else
  {
    funStr = *(m_fitBrowser->m_compositeFunction->getFunction(0));
  }

  Mantid::API::IAlgorithm_sptr alg = 
    Mantid::API::AlgorithmManager::Instance().create("PlotPeakByLogValue");
  alg->initialize();
  alg->setPropertyValue("Input",inputStr.join(";").toStdString());
  alg->setProperty("WorkspaceIndex",m_fitBrowser->workspaceIndex());
  alg->setProperty("StartX",m_fitBrowser->startX());
  alg->setProperty("EndX",m_fitBrowser->endX());
  alg->setPropertyValue("OutputWorkspace",m_fitBrowser->outputName());
  alg->setPropertyValue("Function",funStr);
  if (ui.ckbLogPlot->isChecked())
  {
    std::string logName = ui.cbLogValue->currentText().toStdString();
    alg->setPropertyValue("LogValue",logName);
  }
  alg->setPropertyValue("Minimizer",m_fitBrowser->minimizer());
  alg->setPropertyValue("CostFunction",m_fitBrowser->costFunction());
  if (ui.rbIndividual->isChecked())
  {
    alg->setPropertyValue("FitType","Individual");
  }

  observeFinish(alg);
  alg->executeAsync();
  QDialog::accept();
}

void SequentialFitDialog::populateParameters()
{
  QStringList names;
  for(int i=0;i<m_fitBrowser->m_compositeFunction->nParams();++i)
  {
    names << QString::fromStdString(m_fitBrowser->m_compositeFunction->parameterName(i));
  }
  ui.cbParameter->clear();
  ui.cbParameter->insertItems(0,names);
}

void SequentialFitDialog::functionChanged()
{
  populateParameters();
}


void SequentialFitDialog::finishHandle(const Mantid::API::IAlgorithm* alg)
{
  (void) alg; //Avoid unused warning
  emit needShowPlot();
}

void SequentialFitDialog::showPlot()
{
  std::string wsName = m_fitBrowser->outputName();
  Mantid::API::ITableWorkspace_sptr ws = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(
    Mantid::API::AnalysisDataService::Instance().retrieve(wsName) );
  if (ws)
  {
    if ((ws->columnCount() - 1)/2 != m_fitBrowser->compositeFunction()->nParams()) return;
    Table *t = m_fitBrowser->m_appWindow->mantidUI->importTableWorkspace(QString::fromStdString(wsName));
    if (!t) return;
    QString parName;
    if (m_fitBrowser->compositeFunction()->nFunctions() == 1)
    {
      int i = m_fitBrowser->compositeFunction()->parameterIndex(ui.cbParameter->currentText().toStdString());
      parName = QString::fromStdString(m_fitBrowser->compositeFunction()->getFunction(0)->parameterName(i));
    }
    else
    {
      parName = ui.cbParameter->currentText();
    }
    QStringList colNames;
    colNames << t->name() + "_" + parName << t->name() + "_" + parName + "_Err";
    MultiLayer* ml = m_fitBrowser->m_appWindow->multilayerPlot(t,colNames,ui.cbCurveType->currentIndex());
    // set plot titles
    Graph* g = ml->activeGraph();
    if (g)
    {
      if (ui.ckbLogPlot->isChecked())
      {
        g->setXAxisTitle(ui.cbLogValue->currentText());
      }
      else
      {
        g->setXAxisTitle("Spectra");
      }
      g->setYAxisTitle(parName);
      g->setTitle("");
    }
  }
}

void SequentialFitDialog::helpClicked()
{
  QDesktopServices::openUrl(QUrl("http://www.mantidproject.org/PlotPeakByLogValue"));
}

/**
  * Slot. Called in response to QTableWidget's cellChanged signal.
  * If the cell contains spectra or workspace index of a workspace
  * make them consistent.
  * @param row :: Row index of the modified cell
  * @param col :: Column index of the modified cell
  */
void SequentialFitDialog::spectraChanged(int row,int col)
{
  if (!ui.ckbLogPlot->isChecked()) return;
  QTableWidgetItem* item = ui.tWorkspaces->item(row,3);
  if (!item) return;
  if ((col == 2 || col == 3) && item->flags().testFlag(Qt::ItemIsEnabled) == true)
  {// it's a workspace
    QString name = ui.tWorkspaces->model()->data(ui.tWorkspaces->model()->index(row,0)).toString();
    Mantid::API::MatrixWorkspace_sptr ws;
    try
    {
      ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
             Mantid::API::AnalysisDataService::Instance().retrieve(name.toStdString())
           );
    }
    catch(...)
    {// 
      return;
    }
    if (!ws) return;
    int wi = ui.tWorkspaces->model()->data(ui.tWorkspaces->model()->index(row,3)).toInt();
    int spec = ui.tWorkspaces->model()->data(ui.tWorkspaces->model()->index(row,2)).toInt();
    Mantid::API::Axis* y = ws->getAxis(1);
    if (wi >= 0 && wi < static_cast<int>(ws->getNumberHistograms()))
    {
      // this prevents infinite loops
      if ( !y->isSpectra() || y->spectraNo(wi) == spec) return;
    }
    else
    {
      // return to the previous state
      col = 2;
    }
    if (col == 3) // changed workspace index
    {
      try
      {
        spec = y->spectraNo(wi);
        //ui.tWorkspaces->model()->setData(ui.tWorkspaces->model()->index(row,2),spec);
        setSpectrum(row,spec);
      }
      catch(...)
      {
        // return to the previous state
        col = 2;
      }
    }
    if (col == 2) // changed spectrum number
    {
      for(int i = 0;i<static_cast<int>(y->length()); ++i)
      {
        if ((*y)(i) == spec)
        {
          //ui.tWorkspaces->model()->setData(ui.tWorkspaces->model()->index(row,3),i);
          setWSIndex(row,i);
          return;
        }
      }
      try
      {
        //ui.tWorkspaces->model()->setData(ui.tWorkspaces->model()->index(row,2),(*y)(0));
        setSpectrum(row,int((*y)(0)));
      }
      catch(...){}
    }
  }
}

void SequentialFitDialog::setSpectrum(int row,int spec)
{
  ui.tWorkspaces->model()->setData(ui.tWorkspaces->model()->index(row,2),spec);
}

void SequentialFitDialog::setWSIndex(int row,int wi)
{
  ui.tWorkspaces->model()->setData(ui.tWorkspaces->model()->index(row,3),wi);
}

int SequentialFitDialog::rowCount()const
{
  return ui.tWorkspaces->rowCount();
}

int SequentialFitDialog::defaultSpectrum()const
{
  return ui.sbSpectrum->value();
}

QString SequentialFitDialog::name(int row)const
{
  return ui.tWorkspaces->model()->data(ui.tWorkspaces->model()->index(row,0)).toString();
}

void SequentialFitDialog::setRange(int row,double from,double to)
{
  QString range = QString::number(from) + ":" + QString::number(to);
  ui.tWorkspaces->model()->setData(ui.tWorkspaces->model()->index(row,2),range);
}

void SequentialFitDialog::plotAgainstLog(bool yes)
{
  ui.btnAddFile->setEnabled(yes);
  ui.btnAddWorkspace->setEnabled(yes);
  ui.btnDelete->setEnabled(yes);
  ui.lblLogValue->setVisible(yes);
  ui.cbLogValue->setVisible(yes);
  ui.lblPeriod->setVisible(yes);
  ui.sbPeriod->setVisible(yes);
  ui.lblSpectrum->setVisible(yes);
  ui.sbSpectrum->setVisible(yes);
  if (yes)
  {// plot agains log value
    ui.tWorkspaces->showColumn(3);
    ui.tWorkspaces->horizontalHeaderItem(2)->setData(Qt::DisplayRole,"Spectrum");
    int spec = defaultSpectrum();
    for(int row = 0; row < rowCount(); ++row)
    {
      setSpectrum(row,spec);
    }
  }
  else
  {// plot against "spectra" axis values
    ui.tWorkspaces->hideColumn(3);
    ui.tWorkspaces->horizontalHeaderItem(2)->setData(Qt::DisplayRole,"Range");
    for(int row = 0; row < rowCount(); ++row)
    {
      Mantid::API::MatrixWorkspace_sptr ws;
      try
      {
        ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
          Mantid::API::AnalysisDataService::Instance().retrieve(name(row).toStdString())
          );
      }
      catch(...)
      {// 
        continue;
      }
      if (!ws) continue;
      Mantid::API::Axis* y = ws->getAxis(1);
      setRange(row,(*y)(0),(*y)(y->length()-1));
    }
  }
}

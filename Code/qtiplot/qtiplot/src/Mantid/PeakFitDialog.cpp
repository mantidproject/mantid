//---------------------------------------
// Includes
//---------------------------------------

#include "PeakFitDialog.h"
#include "PeakPickerTool.h"
#include "../ApplicationWindow.h"
#include "MantidUI.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include <muParser.h>
#include <qcheckbox.h>
#include <qmessagebox.h>
#include <qheaderview.h>
#include <iostream>

//---------------------------------------
// Public member functions
//---------------------------------------

/// Constructor
PeakFitDialog::PeakFitDialog(QWidget* parent,PeakPickerTool* peakTool) :
  QDialog(parent),m_ready(false),m_peakTool(peakTool),m_pressedReturnInExpression(false),
    m_mantidUI(((ApplicationWindow*)parent)->mantidUI)
{
  ui.setupUi(this);
  
  connect( ui.btnFit, SIGNAL(clicked()), this, SLOT(fit()) );
  connect( ui.btnClose, SIGNAL(clicked()), this, SLOT(close()) );
  connect( ui.cbFunction, SIGNAL(currentIndexChanged(const QString&)),this, SLOT(setLayout(const QString&)));
  connect( ui.leExpression, SIGNAL(editingFinished()),this, SLOT(setUserParams()) );
  connect( ui.leExpression, SIGNAL(returnPressed()),this, SLOT(returnPressed()) );

  ui.cbFunction->setCurrentIndex(0);
  ui.tableParams->horizontalHeader()-> setStretchLastSection(true);
  QStringList wsNames = m_mantidUI->getWorkspaceNames();
  ui.cbInWorkspace->insertItems(0,wsNames);
  QString wsName = peakTool->workspaceName();
  int i = ui.cbInWorkspace->findText(wsName);
  if (i >= 0)
    ui.cbInWorkspace->setCurrentIndex(i);
  Mantid::API::MatrixWorkspace_sptr ws = 
    boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
    (m_mantidUI->getWorkspace(wsName));
  if (ws)
  {
    ui.leSpectrum->setValidator(new QIntValidator(0,ws->getNumberHistograms()-1,ui.leSpectrum));
    QString spec = QString::number(peakTool->spec());
    ui.leSpectrum->setText(spec);

    ui.leOutWorkspace->setText(wsName + "_" + spec + "_fit_out");
    ui.leParamTable->setText(wsName + "_" + spec + "_params");
  }
  setLayout(ui.cbFunction->currentText());

}

// Checks if everything is ok and starts fitting
void PeakFitDialog::fit()
{
  // Expression editing is finished by pressing the return key => do not start fitting
  if (m_pressedReturnInExpression)
  {
    m_pressedReturnInExpression = false;
    return;
  }

  if (m_ready)
  {
    //Mantid::API::IAlgorithm_sptr alg = createAlgorithm();
    QString function = ui.cbFunction->currentText();
    if (function == "Gaussian")
    {
      fitPeaks();
    }
    else if (function == "Lorentzian")
    {
      fitPeaks();
    }
    else if (function == "User defined")
    {
      std::cerr<<"Not implemented...\n";
    }
  }
  else
    std::cerr<<"Not ready!\n";
}

void PeakFitDialog::returnPressed()
{
  m_pressedReturnInExpression = true;
}

// Resets the dialog alyout depending on the selected fitting function
void PeakFitDialog::setLayout(const QString& functName)
{
  m_ready = false;
  QStringList params;
  if (functName == "Gaussian")
  {
    ui.lblExpression->hide();
    ui.leExpression->hide();
    params << "BG0" << "BG1" << "Height" << "PeakCentre" << "Sigma";
    m_heightName = "Height";
    m_centreName = "PeakCentre";
    m_widthName = "Sigma";
    m_widthCorrectionFormula = "w/2.35482"; // w stands for the FWHM
    m_backgroundFormula = "BG0+BG1*x";
    m_profileFormula = "Height*exp(-0.5*((x - PeakCentre)/Sigma)^2)";
    m_ready = true;
  }
  else if (functName == "Lorentzian")
  {
    ui.lblExpression->hide();
    ui.leExpression->hide();
    params << "BG0" << "BG1" << "Height" << "PeakCentre" << "HWHM";
    m_heightName = "Height";
    m_centreName = "PeakCentre";
    m_widthName = "HWHM";
    m_widthCorrectionFormula = "w/2";
    m_backgroundFormula = "BG0+BG1*x";
    m_profileFormula = "Height*(HWHM^2/((x-PeakCentre)^2+HWHM^2))";
    m_ready = true;
  }
  else if (functName == "User defined")
  {
    ui.lblExpression->show();
    ui.leExpression->show();
    m_ready = false;
  }

  setParamTable(params);
  for(int i=0;i<params.size();i++)
    m_params.insert(params[i].toStdString(),0.0);

}

// Fills in the parameter table with parameter names
void PeakFitDialog::setParamTable(const QStringList& params)
{
  ui.tableParams->setRowCount(0);
  for(int i=0;i<params.size();i++)
  {
    ui.tableParams->insertRow(i);
    ui.tableParams->setItem(i,0,new QTableWidgetItem(params[i]));
    ui.tableParams->setCellWidget(i,1,new FixedSetter);
  }
}

Mantid::API::IAlgorithm_sptr PeakFitDialog::createAlgorithm()
{
  Mantid::API::IAlgorithm_sptr alg;
  QString function = ui.cbFunction->currentText();
  if (function == "Gaussian")
  {
    alg = m_mantidUI->CreateAlgorithm("Gaussian1D");
    alg->initialize();
  }
  else if (function == "Lorentzian")
  {
    alg = m_mantidUI->CreateAlgorithm("Lorentzian1D");
    alg->initialize();
  }
  else if (function == "User defined")
  {
    alg = m_mantidUI->CreateAlgorithm("UserFunction1D");
    alg->initialize();
  }
  return alg;
}

//---------------------------------------
// Private member functions
//---------------------------------------

/** Static callback function simulating variable initialization
    @param varName The name of a new variable
    @param dlg Pointer to a fake variable
 */
double* AddVariable(const char *varName, void *pvar)
{
    return (double*)pvar;
}

void PeakFitDialog::setUserParams()
{
  QStringList params;
  mu::Parser expr;
  double var = 2.;
  expr.SetVarFactory(AddVariable, &var);
  expr.SetExpr(ui.leExpression->text().toStdString());
  expr.Eval();

  mu::varmap_type variables = expr.GetVar();
  mu::varmap_type::const_iterator item = variables.begin();

  bool existsX = false;
  for (; item!=variables.end(); ++item)
  {
    if (item->first == "x") existsX = true;
    else
      params << QString::fromStdString(item->first) ;
  }

  if (existsX)
  {
    setParamTable(params);
    m_ready = true;
  }
  else
  {
    m_ready = false;
    QMessageBox::critical(this,tr("MantidPlot - Error"),
      tr("A user defined fitting function must contain x variable."));

  }
}

bool PeakFitDialog::isFixed(int i)const
{
  if (i < paramCount()) return ((FixedSetter*)ui.tableParams->cellWidget(i,1))->isChecked();
  return false;
}

std::string PeakFitDialog::getValue(int i)const
{
  if (i < paramCount()) return ((FixedSetter*)ui.tableParams->cellWidget(i,1))->getValue().toStdString();
  return "";
}

std::string PeakFitDialog::getValue(const std::string& name)const
{
  for(int i=0;i<paramCount();i++)
    if (getName(i) == name) return getValue(i);
  return "";
}

// get name of parameter i
std::string PeakFitDialog::getName(int i)const
{
  if (i < paramCount()) return ui.tableParams->item(i,0)->text().toStdString();
  return "";
}

// Construct a list of fixed parameters
QString PeakFitDialog::getFixedList()const
{
  QStringList str;
  for(int i=0;i<paramCount();i++)
    if (isFixed(i))
      str << QString::fromStdString(getName(i));
  return str.join(",");
}

// Fit peaks 
void PeakFitDialog::fitPeaks()
{
    const QList<PeakRangeMarker::PeakParams>& peaks = m_peakTool->marker()->params();
    if ( peaks.size() == 0 )
    {
      QMessageBox::critical(this,tr("MantidPlot - Error"),
        tr("The list of peaks is empty."));
      return;
    }
    // Ctreate algorithm according to the selected profile
    Mantid::API::IAlgorithm_sptr alg = createAlgorithm();
    alg->initialize();
    alg->setPropertyValue("InputWorkspace",ui.cbInWorkspace->currentText().toStdString());
    alg->setPropertyValue("WorkspaceIndex",ui.leSpectrum->text().toStdString());
    QString fixed = getFixedList();
    if (!fixed.isEmpty())
      alg->setPropertyValue("Fix",fixed.toStdString());

    Mantid::API::MatrixWorkspace_sptr inputW = alg->getProperty("InputWorkspace");
    int spec = alg->getProperty("WorkspaceIndex");
    Mantid::API::MatrixWorkspace_sptr outputW = 
      Mantid::API::WorkspaceFactory::Instance().create(inputW,3,inputW->readX(spec).size(),inputW->readY(spec).size());
    outputW->dataX(0) = inputW->readX(spec);
    outputW->dataX(1) = inputW->readX(spec);
    outputW->dataX(2) = inputW->readX(spec);
    Mantid::API::ITableWorkspace_sptr outParams = 
      Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
    for(int i=0;i<paramCount();i++)
      outParams->addColumn("double",getName(i));

    // Find the minimum height. 1/100-th of it will be used as a threshould for output
    double dh = peaks.front().height;
    for(int i=1;i<peaks.size();i++)
      if ( dh > peaks[i].height )
        dh = peaks[i].height;
    dh /= 100.;

    // Loop over the selected peaks
    for(int i=0;i<peaks.size();i++)
    {
      if ( i > 0 )// create a new algorithm for each peak
      {
        alg = createAlgorithm();
        alg->initialize();
        alg->setPropertyValue("InputWorkspace",ui.cbInWorkspace->currentText().toStdString());
        alg->setPropertyValue("WorkspaceIndex",ui.leSpectrum->text().toStdString());
        fixed = getFixedList();
        if (!fixed.isEmpty())
          alg->setPropertyValue("Fix",fixed.toStdString());
      }

      // set the parameters which are not peak centre, height or width
      for(int j=0;j<paramCount();j++)
      {
        std::string name = getName(j);
        if (name != m_heightName && name != m_centreName && name != m_widthName)
        {
          std::string val = getValue(j);
          if (!val.empty()) alg->setPropertyValue(name,val);
        }
      }

      // set the centre, height and width of the peak
      std::string val = getValue(m_heightName);
      if (!val.empty()) alg->setPropertyValue(m_heightName,val);
      else
        alg->setPropertyValue(m_heightName,QString::number(peaks[i].height).toStdString());

      val = getValue(m_centreName);
      if (!val.empty()) alg->setPropertyValue(m_centreName,val);
      else
        alg->setPropertyValue(m_centreName,QString::number(peaks[i].centre).toStdString());

      val = getValue(m_widthName);
      if (!val.empty()) alg->setPropertyValue(m_widthName,val);
      else
      {
        // --- width param correction ---
        double widthParam;
        if (!m_widthCorrectionFormula.empty())
        {
          mu::Parser parser;
          parser.SetExpr(m_widthCorrectionFormula);
          double width = peaks[i].width;
          parser.DefineVar("w",&width);
          widthParam = parser.Eval();
        }
        else
          widthParam = peaks[i].width;
        alg->setPropertyValue(m_widthName,QString::number(widthParam).toStdString());
      }

      double startX = peaks[i].centre - 6*peaks[i].width;
      double endX   = peaks[i].centre + 6*peaks[i].width;

      alg->setPropertyValue("StartX",QString::number(startX).toStdString());
      alg->setPropertyValue("EndX",QString::number(endX).toStdString());

      alg->execute();

      Mantid::API::TableRow row = outParams->appendRow();

      double x;
      mu::Parser backgroundParser;
      backgroundParser.SetExpr(m_backgroundFormula);
      mu::Parser profileParser;
      profileParser.SetExpr(m_profileFormula);
      for(int j=0;j<paramCount();j++)
      {
        std::string name = getName(j);
        double *pval = &m_params[name];
        *pval = alg->getProperty(name);
        backgroundParser.DefineVar(name,pval);
        backgroundParser.DefineVar("x",&x);
        profileParser.DefineVar(name,pval);
        profileParser.DefineVar("x",&x);
        row << *pval ;
      }

      Mantid::MantidVec& Y0 = outputW->dataY(0);
      Mantid::MantidVec& Y1 = outputW->dataY(1);
      Mantid::MantidVec& D = outputW->dataY(2);
      for(int j=0;j<outputW->readY(0).size();j++)
      {
        try
        {
          x = outputW->readX(0)[j];
          double y = profileParser.Eval();
          double y0 = backgroundParser.Eval();
          if (fabs(y) > dh)
            Y1[j] += y0 + y;
          if ( i == peaks.size() - 1 )
          {
            Y0[j] = inputW->readY(spec)[j];
            D[j]  = Y0[j] - Y1[j];
          }
        }
        catch(mu::Parser::exception_type& e)
        {
          std::cerr<<e.GetMsg()<<'\n';
        }
      }

    }

    // Store the output workspace
    Mantid::API::AnalysisDataService::Instance().addOrReplace(ui.leOutWorkspace->text().toStdString(),outputW);
    Mantid::API::AnalysisDataService::Instance().addOrReplace(ui.leParamTable->text().toStdString(),outParams);

}

//---------------------------------------
// Includes
//---------------------------------------

#include "PeakFitDialog.h"
#include "PeakPickerTool1D.h"
#include "../ApplicationWindow.h"
#include "MantidUI.h"
#include "MantidCurve.h"
#include "UserFitFunctionDialog.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/AlgorithmFactory.h"
#include <muParser.h>
#include <qcheckbox.h>
#include <qmessagebox.h>
#include <qheaderview.h>
#include <iostream>

//---------------------------------------
// Public member functions
//---------------------------------------

/// Constructor
PeakFitDialog::PeakFitDialog(QWidget* parent,PeakPickerTool1D* peakTool) :
  QDialog(parent),m_ready(false),m_peakTool(peakTool),m_pressedReturnInExpression(false),
    m_mantidUI(((ApplicationWindow*)parent)->mantidUI)
{
  ui.setupUi(this);
  
  connect( ui.btnFit, SIGNAL(clicked()), this, SLOT(fit()) );
  connect( ui.btnClose, SIGNAL(clicked()), this, SLOT(close()) );
  connect( ui.cbFunction, SIGNAL(currentIndexChanged(const QString&)),this, SLOT(setLayout(const QString&)));
  connect( ui.leExpression, SIGNAL(editingFinished()),this, SLOT(setUserParams()) );
  connect( ui.leExpression, SIGNAL(returnPressed()),this, SLOT(returnPressed()) );
  connect( ui.cbPTCentre, SIGNAL(currentIndexChanged ( const QString &)),this, SLOT(centreNameChanged ( const QString &)) );
  connect( ui.cbPTHeight, SIGNAL(currentIndexChanged ( const QString &)),this, SLOT(heightNameChanged ( const QString &)) );
  connect( ui.cbPTWidth, SIGNAL(currentIndexChanged ( const QString &)),this, SLOT(widthNameChanged ( const QString &)) );
  connect( ui.btnConstruct, SIGNAL(clicked()), this, SLOT(startUserFitFunctionDialog()) );
  connect( ui.chbEnableEditIO, SIGNAL(stateChanged(int)),this,SLOT(editIOParams(int)));

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
  editIOParams(Qt::Unchecked);

  //typedef std::vector<Mantid::API::Algorithm_descriptor> AlgNamesType;
  //AlgNamesType algs = Mantid::API::AlgorithmFactory::Instance().getDescriptors();
  //for(AlgNamesType::const_iterator a = algs.begin();a != algs.end(); a++)
  //{
  //  std::cerr<<a->category<<':'<<a->name<<'\n';
  //  if (a->category == "CurveFitting")
  //  {
  //    ui.cbFunction->addItem(QString::fromStdString(a->name));
  //  }
  //}


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
    fitPeaks();
    close();
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
  ui.cbPTCentre->clear();
  ui.cbPTHeight->clear();
  ui.cbPTWidth->clear();
  m_ready = false;
  QStringList params;
  if (functName == "User" || functName == "UserFunction1D")
  {
    ui.lblExpression->show();
    ui.leExpression->show();
    ui.btnConstruct->show();
    ui.leWidthFormula->setText("");
    ui.leWidthFormula->setEnabled(true);
    ui.cbPTCentre->setEnabled(true);
    ui.cbPTHeight->setEnabled(true);
    ui.cbPTWidth->setEnabled(true);
    m_backgroundFormula = "0";
    setUserParams();
  }
  else
  {
    if (functName == "Gaussian")
    {
      params << "BG0" << "BG1" << "Height" << "PeakCentre" << "Sigma";
      m_heightName = "Height";
      m_centreName = "PeakCentre";
      m_widthName = "Sigma";
      m_widthCorrectionFormula = "w/2.35482"; // w stands for the FWHM
      m_backgroundFormula = "BG0+BG1*x";
      m_profileFormula = "Height*exp(-0.5*((x - PeakCentre)/Sigma)^2)";
    }
    else if (functName == "Lorentzian")
    {
      params << "BG0" << "BG1" << "Height" << "PeakCentre" << "HWHM";
      m_heightName = "Height";
      m_centreName = "PeakCentre";
      m_widthName = "HWHM";
      m_widthCorrectionFormula = "w/2";
      m_backgroundFormula = "BG0+BG1*x";
      m_profileFormula = "Height*(HWHM^2/((x-PeakCentre)^2+HWHM^2))";
    }
    else
    {
    }

    m_ready = true;
    ui.lblExpression->hide();
    ui.leExpression->hide();
    ui.btnConstruct->hide();

    ui.leWidthFormula->setText(QString::fromStdString(m_widthCorrectionFormula));
    ui.leWidthFormula->setEnabled(false);

    ui.cbPTCentre->addItem(QString::fromStdString(m_centreName));
    ui.cbPTCentre->setEnabled(false);

    ui.cbPTHeight->addItem(QString::fromStdString(m_heightName));
    ui.cbPTHeight->setEnabled(false);

    ui.cbPTWidth->addItem(QString::fromStdString(m_widthName));
    ui.cbPTWidth->setEnabled(false);

    setParamTable(params);
  }

}

// Fills in the parameter table with parameter names
void PeakFitDialog::setParamTable(const QStringList& params)
{
  ui.tableParams->setRowCount(0);
  m_params.clear();
  for(int i=0;i<params.size();i++)
  {
    ui.tableParams->insertRow(i);
    ui.tableParams->setItem(i,0,new QTableWidgetItem(params[i]));
    ui.tableParams->setCellWidget(i,1,new FixedSetter);
    m_params.insert(params[i].toStdString(),0.0);
  }
}

Mantid::API::IAlgorithm_sptr PeakFitDialog::createAlgorithm()
{
  Mantid::API::IAlgorithm_sptr alg;
  QString function = ui.cbFunction->currentText();
  if (function == "Gaussian")
  {
    alg = m_mantidUI->CreateAlgorithm("Gaussian1D");
  }
  else if (function == "Lorentzian")
  {
    alg = m_mantidUI->CreateAlgorithm("Lorentzian1D");
  }
  else if (function == "User")
  {
    alg = m_mantidUI->CreateAlgorithm("UserFunction1D");
  }
  else
  {
    alg = m_mantidUI->CreateAlgorithm(function);
  }
  alg->initialize();
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

void PeakFitDialog::centreNameChanged ( const QString &str)
{
  m_centreName = str.toStdString();
}

void PeakFitDialog::heightNameChanged ( const QString &str)
{
  m_heightName = str.toStdString();
}

void PeakFitDialog::widthNameChanged ( const QString &str)
{
  m_widthName = str.toStdString();
}

void PeakFitDialog::editIOParams(int state)
{
  if (state == Qt::Checked)
  {
    ui.cbInWorkspace->setEnabled(true);
    ui.leSpectrum->setEnabled(true);
    ui.leOutWorkspace->setEnabled(true);
    ui.leParamTable->setEnabled(true);
  }
  else
  {
    ui.cbInWorkspace->setEnabled(false);
    ui.leSpectrum->setEnabled(false);
    ui.leOutWorkspace->setEnabled(false);
    ui.leParamTable->setEnabled(false);
  }
}

void PeakFitDialog::startUserFitFunctionDialog()
{
  UserFitFunctionDialog dlg((QWidget*)this);
  if (dlg.exec() == QDialog::Accepted)
  {
    ui.leExpression->setText( dlg.expression() );
    QString peakP = dlg.peakParams();
    if (!peakP.isEmpty())
    {
      setPeakParams(peakP);
      m_widthCorrectionFormula = dlg.widthFormula().toStdString();
      ui.leWidthFormula->setText(QString::fromStdString(m_widthCorrectionFormula));
    }
    setUserParams(true);
  }
}

void PeakFitDialog::setUserParams()
{
  setUserParams(false);
}

void PeakFitDialog::setUserParams(bool keepParamNames)
{
  std::string expression = ui.leExpression->text().toStdString();
  if (expression.empty()) 
  {
    setParamTable(QStringList());
    return;
  }

  m_profileFormula = expression;

  QStringList params;
  mu::Parser expr;
  double var = 2.;
  expr.SetVarFactory(AddVariable, &var);
  try
  {
    expr.SetExpr(expression);
    expr.Eval();
  }
  catch(...)
  {
    QMessageBox::critical(this,"Mantid - Error","The expression contains errors.");
    return;
  }

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

  // Save peak parameter names since combobox manipulations below change them
  std::string temp_centre = m_centreName;
  std::string temp_height = m_heightName;
  std::string temp_width = m_widthName;

  ui.cbPTCentre->clear();
  ui.cbPTHeight->clear();
  ui.cbPTWidth->clear();

  ui.cbPTCentre->addItem("?");
  ui.cbPTHeight->addItem("?");
  ui.cbPTWidth->addItem("?");

  ui.cbPTCentre->addItems(params);
  ui.cbPTHeight->addItems(params);
  ui.cbPTWidth->addItems(params);

  if (!keepParamNames)
  {
    m_heightName = "";
    m_centreName = "";
    m_widthName = "";

    foreach(QString s,params)
    {
      QString us = s.toUpper();
      if (m_centreName.empty())
      {
        if (us == "X0" || us.contains("CENTRE"))
        {
          m_centreName = s.toStdString();
        }
      }
      if (m_heightName.empty())
      {
        if (us == "H" || us.contains("HEI") || us == "HI" )
        {
          m_heightName = s.toStdString();
        }
      }
      if (m_widthName.empty())
      {
        if (us == "W" || us.contains("WID"))
        {
          m_widthName = s.toStdString();
        }
      }
    }
  }
  else
  {// restore the names
    m_centreName = temp_centre;
    m_heightName = temp_height;
    m_widthName = temp_width;
  }

  if (!m_centreName.empty())
    ui.cbPTCentre->setCurrentIndex(ui.cbPTCentre->findText(QString::fromStdString(m_centreName)));

  if (!m_heightName.empty())
    ui.cbPTHeight->setCurrentIndex(ui.cbPTHeight->findText(QString::fromStdString(m_heightName)));

  if (!m_widthName.empty())
  {
    ui.cbPTWidth->setCurrentIndex(ui.cbPTWidth->findText(QString::fromStdString(m_widthName)));
    int i = ui.cbPTWidth->findText(QString::fromStdString(m_widthName));
  }
}

void PeakFitDialog::setPeakParams(const QString& str)
{
  QStringList list = str.split(",");

  m_heightName = "";
  m_centreName = "";
  m_widthName = "";

  if (list.size() > 0)
  {
    QString s = list[0];
    m_centreName = s.toStdString();
  }

  if (list.size() > 1)
  {
    QString s = list[1];
    m_heightName = s.toStdString();
  }

  if (list.size() > 2)
  {
    QString s = list[2];
    m_widthName = s.toStdString();
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
    const QList<PeakRangeMarker1D::PeakParams>& peaks = m_peakTool->marker()->params();
    if ( peaks.size() == 0 )
    {
      QMessageBox::critical(this,tr("MantidPlot - Error"),
        tr("The list of peaks is empty."));
      return;
    }

    bool isUser = ui.cbFunction->currentText() == "User";

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

      double startX = peaks[i].centre - peaks[i].width/2;
      double endX   = peaks[i].centre + peaks[i].width/2;

      if (startX == endX)
      {
        QMessageBox::critical(this,"MantidPlot - Error","Zero width is set for peak at "+QString::number(peaks[i].centre));
        return;
      }

      alg->setPropertyValue("StartX",QString::number(startX).toStdString());
      alg->setPropertyValue("EndX",QString::number(endX).toStdString());

      // set the centre, height and width of the peak
      double centreParam = peaks[i].centre;
      double heightParam = peaks[i].height;
      double widthParam = peaks[i].width/6;

      // analyze the spectrum to find more accurate height and width
      {
        const Mantid::MantidVec& X0 = inputW->readX(spec);
        const Mantid::MantidVec& Y0 = inputW->readY(spec);
        int minX = 0, maxX = Y0.size();
        for(int i=0;i<Y0.size();i++)
        {
          if (X0[i] <= startX) minX = i;
          if (X0[i] >= endX)
          {
            maxX = i;
            break;
          }
        }

        int ih = minX+1, iw = minX;
        double h0 = (Y0[minX] + Y0[maxX-1])/2;
        double hmax = Y0[ih]-h0;
        for(int i=minX;i<maxX;i++)
        {
          double h = Y0[i] - h0;
          if (hmax < h)
          {
            hmax = h;
            ih = i;
            double hmax2 = hmax / 2;
            while(Y0[iw] - h0 < hmax2 && iw < ih-1) iw++;
          }
        }
        centreParam = X0[ih];
        heightParam = hmax;
        if (ih - iw > 1)
          widthParam = (X0[ih]-X0[iw])*2;
      }

      // --- width param correction ---
      if (!m_widthCorrectionFormula.empty())
      {
        mu::Parser parser;
        parser.SetExpr(m_widthCorrectionFormula);
        double width = widthParam;
        parser.DefineVar("w",&width);
        widthParam = parser.Eval();
      }

      std::string fixed_height_val = getValue(m_heightName);
      std::string fixed_centre_val = getValue(m_centreName);
      std::string fixed_width_val = getValue(m_widthName);

      if (isUser)
      {
        alg->setPropertyValue("Function",ui.leExpression->text().toStdString());
        QStringList initParams;
        for(int j=0;j<paramCount();j++)
        {
          std::string name = getName(j);
          if (name != m_heightName && name != m_centreName && name != m_widthName)
          {
            std::string val = getValue(j);
            if (!val.empty()) initParams << QString::fromStdString(name+"="+val);
          }
        }
        if (!m_heightName.empty())
        {
          if (!fixed_height_val.empty()) 
            initParams << QString::fromStdString(m_heightName+"="+fixed_height_val);
          else
            initParams << QString::fromStdString(m_heightName+"="+QString::number(heightParam).toStdString());
        }

        if (!m_centreName.empty())
        {
          if (!fixed_centre_val.empty()) 
            initParams << QString::fromStdString(m_centreName+"="+fixed_centre_val);
          else
            initParams << QString::fromStdString(m_centreName+"="+QString::number(centreParam).toStdString());
        }

        if (!m_widthName.empty())
        {
          if (!fixed_width_val.empty()) 
            initParams << QString::fromStdString(m_widthName+"="+fixed_width_val);
          else
            initParams << QString::fromStdString(m_widthName+"="+QString::number(widthParam).toStdString());
        }

        alg->setPropertyValue("InitialParameters",initParams.join(",").toStdString());
      }
      else
      {
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

        // set centre, height and width
        if (!m_heightName.empty())
        {
          if (!fixed_height_val.empty())
            alg->setPropertyValue(m_heightName,fixed_height_val);
          else
            alg->setPropertyValue(m_heightName,QString::number(heightParam).toStdString());
        }

        if (!m_centreName.empty())
        {
          if (!fixed_centre_val.empty())
            alg->setPropertyValue(m_centreName,fixed_centre_val);
          else
            alg->setPropertyValue(m_centreName,QString::number(centreParam).toStdString());
        }

        if (!m_widthName.empty())
        {
          if (!fixed_width_val.empty())
            alg->setPropertyValue(m_widthName,fixed_width_val);
          else
            alg->setPropertyValue(m_widthName,QString::number(widthParam).toStdString());
        }
      }

      alg->execute();

      if (!alg->isExecuted())
      {
        QMessageBox::critical(this,"MantidPlot - Error","The fitting algorithm failed.");
        return;
      }

      Mantid::API::TableRow row = outParams->appendRow();

      double x;
      mu::Parser backgroundParser;
      backgroundParser.SetExpr(m_backgroundFormula);
      backgroundParser.DefineVar("x",&x);
      mu::Parser profileParser;
      profileParser.SetExpr(m_profileFormula);
      profileParser.DefineVar("x",&x);
      for(int j=0;j<paramCount();j++)
      {
        std::string name = getName(j);
        double *pval = &m_params[name];
        *pval = alg->getProperty(name);
        backgroundParser.DefineVar(name,pval);
        profileParser.DefineVar(name,pval);
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
          if (x < startX) continue;
          if (x > endX) break;
          double y = profileParser.Eval();
          double y0 = backgroundParser.Eval();
          Y1[j] = y0 + y;
          Y0[j] = inputW->readY(spec)[j];
          D[j]  = Y0[j] - Y1[j];
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

    MantidCurve* c1 = new MantidCurve(m_peakTool->workspaceName()+QString("-fit-")+QString::number(m_peakTool->spec()),
      ui.leOutWorkspace->text(),m_peakTool->graph(),"spectra",1,false);
//    connect(m_mantidUI,SIGNAL(workspace_removed(const QString&)),c1,SLOT(workspaceRemoved(const QString&)));

    MantidCurve* c2 = new MantidCurve(m_peakTool->workspaceName()+QString("-res-")+QString::number(m_peakTool->spec()),
      ui.leOutWorkspace->text(),m_peakTool->graph(),"spectra",2,false);
//    connect(m_mantidUI,SIGNAL(workspace_removed(const QString&)),c2,SLOT(workspaceRemoved(const QString&)));

}

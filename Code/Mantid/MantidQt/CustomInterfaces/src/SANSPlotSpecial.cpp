#include "MantidQtCustomInterfaces/SANSPlotSpecial.h"

#include "MantidKernel/PhysicalConstants.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/IFunction.h"

#include "MantidQtMantidWidgets/RangeSelector.h"

#include <QLineEdit>
#include <QDesktopServices>
#include <QUrl>
#include "qwt_plot_curve.h"

namespace MantidQt
{
namespace CustomInterfaces
{

SANSPlotSpecial::SANSPlotSpecial(QWidget *parent) : 
  QFrame(parent), m_rangeSelector(NULL), m_transforms(), m_current(""),
  m_dataCurve(new QwtPlotCurve()), m_linearCurve(new QwtPlotCurve()), m_rearrangingTable(false)
{
  m_uiForm.setupUi(this);
  initLayout();
}

SANSPlotSpecial::~SANSPlotSpecial()
{
  //
}

void SANSPlotSpecial::rangeChanged(double low, double high)
{
  if ( ! m_workspaceIQT ) { return; }

  Mantid::API::IAlgorithm_sptr fit = Mantid::API::AlgorithmManager::Instance().create("Fit");
  fit->initialize();
  fit->setPropertyValue("Function", "name=UserFunction, Formula=Intercept+Gradient*x");
  fit->setProperty<Mantid::API::MatrixWorkspace_sptr>("InputWorkspace", m_workspaceIQT);
  fit->setPropertyValue("Output", "__sans_isis_display_linear");
  fit->setProperty<double>("StartX", low);
  fit->setProperty<double>("EndX", high);
  fit->execute();

  if ( ! fit->isExecuted() ) { return; }

  m_workspaceLinear = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
    (Mantid::API::AnalysisDataService::Instance().retrieve("__sans_isis_display_linear_Workspace"));
  m_linearCurve = plotMiniplot(m_linearCurve, m_workspaceLinear, 1);

  QPen fitPen(Qt::red, Qt::SolidLine);
  m_linearCurve->setPen(fitPen);
  m_uiForm.plotWindow->replot();

  Mantid::API::IFunction_sptr func = fit->getProperty("Function");

  double chisqrd = fit->getProperty("OutputChi2overDoF");

  m_derivatives["Intercept"]->setText(QString::number(func->getParameter("Intercept")));
  m_derivatives["Gradient"]->setText(QString::number(func->getParameter("Gradient")));
  m_derivatives["Chi Squared"]->setText(QString::number(chisqrd));

  calculateDerivatives();
}

void SANSPlotSpecial::plot()
{
  // validate input
  if ( ! validatePlotOptions() )
  {
    return;
  }
  // Run iq transform algorithm
  m_workspaceIQT = runIQTransform();
  if ( m_workspaceIQT )
  {
    // plot data to the plotWindow
    m_dataCurve = plotMiniplot(m_dataCurve, m_workspaceIQT);
    // update fields of table of "derived" values?
    QPair<QStringList, QList<QPair<int, int> > > deriv = m_transforms[m_uiForm.cbPlotType->currentText()]->derivatives();
    tableDisplay(deriv.first, deriv.second);
    calculateDerivatives();
  }
}

void SANSPlotSpecial::updateAxisLabels(const QString & value)
{
  if ( m_current != "" )
  {
    foreach ( QWidget* item, m_transforms[m_current]->xWidgets() )
    {
      m_uiForm.layoutXAxis->removeWidget(item);
      delete item;
    }
    foreach ( QWidget* item, m_transforms[m_current]->yWidgets() )
    {
      m_uiForm.layoutYAxis->removeWidget(item);
      delete item;
    }
    m_transforms[m_current]->init();
  }

  foreach ( QWidget* item, m_transforms[value]->xWidgets() )
  {
    m_uiForm.layoutXAxis->addWidget(item);
  }
  foreach ( QWidget* item, m_transforms[value]->yWidgets() )
  {
    m_uiForm.layoutYAxis->addWidget(item);
  }

  m_current = value;
}

void SANSPlotSpecial::clearTable()
{
  // Removes items from the G Derived and I Derived columns
  // deleting the labels but preserving the actual objects
  int nrows = m_uiForm.tbDerived->rowCount();
  for ( int i = 0; i < nrows; i++ )
  {
    m_uiForm.tbDerived->setItem(i, SANSPlotSpecial::GradientLabels, new QTableWidgetItem(*m_emptyCell));
    m_uiForm.tbDerived->setItem(i, SANSPlotSpecial::GradientUnits, new QTableWidgetItem(*m_emptyCell));
    m_uiForm.tbDerived->setItem(i, SANSPlotSpecial::InterceptLabels, new QTableWidgetItem(*m_emptyCell));
    m_uiForm.tbDerived->setItem(i, SANSPlotSpecial::InterceptUnits, new QTableWidgetItem(*m_emptyCell));
    m_uiForm.tbDerived->takeItem(i, SANSPlotSpecial::GradientDerived);
    m_uiForm.tbDerived->takeItem(i, SANSPlotSpecial::InterceptDerived);
  }

  while ( m_uiForm.tbDerived->rowCount() > 3 )
  {
    m_uiForm.tbDerived->removeRow(3);
  } 
}

void SANSPlotSpecial::calculateDerivatives()
{
  m_rearrangingTable = true;

  Transform* transform = m_transforms[m_uiForm.cbPlotType->currentText()];
  Transform::TransformType type = transform->type();
  double temp = 0.0;
  const double gradient = m_derivatives["Gradient"]->text().toDouble();
  const double intercept = m_derivatives["Intercept"]->text().toDouble();
  switch ( type )
  {
  case Transform::GuinierSpheres:
    // Gradient = -(Rg**2)/3 = -(R**2)/5
    temp = std::sqrt(3 * std::abs(gradient) );
    m_derivatives["Rg"]->setText(QString::number(temp));
    temp = std::sqrt(5 * std::abs(gradient) );
    m_derivatives["R"]->setText(QString::number(temp));
    // Intercept = M.[(c.(deltarho**2) / (NA.d**2)] = M.[(phi.(deltarho**2) / (NA.d)]
    deriveGuinierSpheres();
    break;
  case Transform::GuinierRods:
    // Gradient = -(Rg,xs**2)/2  (note dividing by 2 this time)
    temp = std::sqrt(2 * std::abs(gradient) );
    m_derivatives["Rg,xs"]->setText(QString::number(temp));
    //Intercept (Q**2=0) = Ln[(pi.c.(deltarho**2).ML) / (NA.d**2)]
    deriveGuinierRods();
    break;
  case Transform::GuinierSheets:
    temp = std::sqrt(std::abs(gradient) * 12);
    m_derivatives["T"]->setText(QString::number(temp));
    break;
  case Transform::Zimm:
    // Gradient = (Rg**2)/3 = (R**2)/5
    temp = std::sqrt(3 * std::abs(gradient) / intercept );
    m_derivatives["Rg"]->setText(QString::number(temp));
    temp = std::sqrt(5 * std::abs(gradient) / intercept );
    m_derivatives["R"]->setText(QString::number(temp));
    // Intercept = (1/M).[(NA.d**2) / (c.(deltarho**2)] = (1/M).[(NA.d) / (phi.(deltarho**2)]
    deriveZimm();
    break;
  case Transform::Kratky:
    // Plateau Intercept = [(2.c.M.(deltarho**2)) / (NA.(d**2).(Rg**2))] = [(2.phi.M.(deltarho**2)) / (NA.d.(Rg**2))]
    deriveKratky();
    break;
  case Transform::DebyeBueche:
    temp = std::sqrt( gradient / intercept );
    m_derivatives["Zeta"]->setText(QString::number(temp));
    break;
  case Transform::LogLog:
    temp = - gradient;
    m_derivatives["N"]->setText(QString::number(temp));
    temp = -1 / gradient;
    m_derivatives["V"]->setText(QString::number(temp));
    break;
  case Transform::Porod:
    // Plateau Intercept = [(2.pi.c.(deltarho**2)) / d].(S / V)
    derivePorod();
    break;
  default:
    break;
  }

  m_rearrangingTable = false;
}

void SANSPlotSpecial::tableUpdated(int row, int column)
{
  UNUSED_ARG(row);

  if ( m_rearrangingTable ) { return; }
  if ( ! ( column == SANSPlotSpecial::GradientDerived || column == SANSPlotSpecial::InterceptDerived ) ) { return; }

  calculateDerivatives();
}

void SANSPlotSpecial::clearInterceptDerived()
{
  m_rearrangingTable = true;

  for ( int i = 0; i < m_uiForm.tbDerived->rowCount(); i++ )
  {
    auto wi = m_uiForm.tbDerived->item(i, SANSPlotSpecial::InterceptDerived);
    if (wi)
      wi->setText("");
  }

  m_rearrangingTable = false;
}

void SANSPlotSpecial::scalePlot(double start, double end)
{
  double delta = end - start;
  double limA = start - ( delta / 10 );
  double limB = end + ( delta / 10 );
  m_uiForm.plotWindow->setAxisScale(QwtPlot::xBottom, limA, limB);
  m_uiForm.plotWindow->replot();
}

void SANSPlotSpecial::resetSelectors()
{
  if ( m_dataCurve )
  {
    const double min = m_dataCurve->minXValue();
    const double max = m_dataCurve->maxXValue();
    m_uiForm.plotWindow->setAxisScale(QwtPlot::xBottom, min, max);
    m_rangeSelector->setMinimum(min);
    m_rangeSelector->setMaximum(max);
    m_uiForm.plotWindow->replot();
  }
}

void SANSPlotSpecial::initLayout()
{
  createTransforms();
  setupTable();

  // Setup the cosmetics for the plotWindow
  m_uiForm.plotWindow->setAxisFont(QwtPlot::xBottom, this->font());
  m_uiForm.plotWindow->setAxisFont(QwtPlot::yLeft, this->font());
  m_uiForm.plotWindow->setCanvasBackground(Qt::white);

  // Setup RangeSelector widget for use on the plotWindow
  m_rangeSelector = new MantidWidgets::RangeSelector(m_uiForm.plotWindow);
  connect(m_rangeSelector, SIGNAL(selectionChanged(double, double)), this, SLOT(rangeChanged(double, double)));

  // Scale the plot based on the range selection
  connect(m_rangeSelector, SIGNAL(selectionChangedLazy(double, double)), this, SLOT(scalePlot(double, double)));

  connect(m_uiForm.pbResetRangeSelectors, SIGNAL(clicked()), this, SLOT(resetSelectors()));

  // Other signal/slot connections
  connect(m_uiForm.pbPlot, SIGNAL(clicked()), this, SLOT(plot()));
  connect(m_uiForm.cbBackground, SIGNAL(currentIndexChanged(int)), m_uiForm.swBackground, SLOT(setCurrentIndex(int)));
  connect(m_uiForm.cbPlotType, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(updateAxisLabels(const QString &)));
  connect(m_uiForm.tbDerived, SIGNAL(cellChanged(int, int)), this, SLOT(tableUpdated(int, int)));
  connect(m_uiForm.pbClearIDerived, SIGNAL(clicked()), this, SLOT(clearInterceptDerived()));
  updateAxisLabels(m_uiForm.cbPlotType->currentText());
}

Mantid::API::MatrixWorkspace_sptr SANSPlotSpecial::runIQTransform()
{
  // Run the IQTransform algorithm for the current settings on the GUI
  Mantid::API::IAlgorithm_sptr iqt = Mantid::API::AlgorithmManager::Instance().create("IQTransform");
  iqt->initialize();
  try
  {
    iqt->setPropertyValue("InputWorkspace", m_uiForm.wsInput->currentText().toStdString());
  } catch ( std::invalid_argument & )
  {
    m_uiForm.lbPlotOptionsError->setText("Selected input workspace is not appropriate for the IQTransform algorithm. Please refer to the documentation for guidelines.");
    return Mantid::API::MatrixWorkspace_sptr();
  }
  iqt->setPropertyValue("OutputWorkspace", "__sans_isis_display_iqt");
  iqt->setPropertyValue("TransformType", m_uiForm.cbPlotType->currentText().toStdString());
  
  if ( m_uiForm.cbBackground->currentText() == "Value" )
  { 
    iqt->setProperty<double>("BackgroundValue", m_uiForm.dsBackground->value());
  }
  else
  { 
    iqt->setPropertyValue("BackgroundWorkspace", m_uiForm.wsBackground->currentText().toStdString());
  }

  if ( m_uiForm.cbPlotType->currentText() == "General" )
  {
    std::vector<double> constants = m_transforms["General"]->functionConstants();
    iqt->setProperty("GeneralFunctionConstants", constants);
  }

  iqt->execute();

  Mantid::API::MatrixWorkspace_sptr result =
    boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve("__sans_isis_display_iqt"));
  return result;
}

void SANSPlotSpecial::tableDisplay(QStringList properties, QList<QPair<int, int> > positions)
{
  m_rearrangingTable = true;

  clearTable();

  QList<QPair<int, int> >::iterator pos = positions.begin();
  for ( QStringList::iterator it = properties.begin(); it != properties.end(); ++it, ++pos )
  {
    int row = (*pos).first;
    if ( row > ( m_uiForm.tbDerived->rowCount() - 1 ) )
    {
      m_uiForm.tbDerived->insertRow(row);
    }
    int column = (*pos).second;
    QTableWidgetItem* lblItm = new QTableWidgetItem(*m_emptyCell);
    lblItm->setToolTip(m_derivatives[(*it)]->toolTip());
    lblItm->setText((*it));
    QTableWidgetItem* unitItm = new QTableWidgetItem(*m_emptyCell);
    unitItm->setText(m_units[(*it)]);
    m_uiForm.tbDerived->setItem(row, column, lblItm);
    m_uiForm.tbDerived->setItem(row, column+1, m_derivatives[(*it)]);
    m_uiForm.tbDerived->setItem(row, column+2, unitItm);
  }

  m_rearrangingTable = false;
}

bool SANSPlotSpecial::validatePlotOptions()
{
  bool valid = true;
  m_uiForm.lbPlotOptionsError->setText("");
  QString error = "";

  if ( m_uiForm.wsInput->currentText() == "" )
  {
    error += "Please select an input workspace.\n";
    valid = false;
  }

  if ( m_uiForm.cbBackground->currentText() == "Workspace" &&
    m_uiForm.wsBackground->currentText() == "" )
  {
    error += "Please select a background workspace.\n";
    valid = false;
  }

  if ( m_uiForm.cbPlotType->currentText() == "General" )
  {
    std::vector<double> params = m_transforms["General"]->functionConstants();
    if ( params.size() != 10 )
    {
      error += "Constants for general function not provided.";
      valid = false;
    }
  }

  m_uiForm.lbPlotOptionsError->setText(error.trimmed());
  return valid;
}

void SANSPlotSpecial::createTransforms()
{
  m_transforms.clear();

  m_transforms["Guinier (spheres)"] = new Transform(Transform::GuinierSpheres);
  m_uiForm.cbPlotType->addItem("Guinier (spheres)");
  m_transforms["Guinier (rods)"] = new Transform(Transform::GuinierRods);
  m_uiForm.cbPlotType->addItem("Guinier (rods)");
  m_transforms["Guinier (sheets)"] = new Transform(Transform::GuinierSheets);
  m_uiForm.cbPlotType->addItem("Guinier (sheets)");
  m_transforms["Zimm"] = new Transform(Transform::Zimm);
  m_uiForm.cbPlotType->addItem("Zimm");
  m_transforms["Debye-Bueche"] = new Transform(Transform::DebyeBueche);
  m_uiForm.cbPlotType->addItem("Debye-Bueche");
  m_transforms["Holtzer"] = new Transform(Transform::Holtzer);
  m_uiForm.cbPlotType->addItem("Holtzer");
  m_transforms["Kratky"] = new Transform(Transform::Kratky);
  m_uiForm.cbPlotType->addItem("Kratky");
  m_transforms["Porod"] = new Transform(Transform::Porod);
  m_uiForm.cbPlotType->addItem("Porod");
  m_transforms["Log-Log"] = new Transform(Transform::LogLog);
  m_uiForm.cbPlotType->addItem("Log-Log");
  m_transforms["General"] = new Transform(Transform::General);
  m_uiForm.cbPlotType->addItem("General");
}

void SANSPlotSpecial::setupTable()
{
  m_emptyCell = new QTableWidgetItem();
  m_emptyCell->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled );
  m_uiForm.tbDerived->setItemPrototype(m_emptyCell);

  m_derivatives["Gradient"] = new QTableWidgetItem(*m_emptyCell);
  m_derivatives["Intercept"] = new QTableWidgetItem(*m_emptyCell);
  m_derivatives["Chi Squared"] = new QTableWidgetItem(*m_emptyCell);

  QTableWidgetItem* lbl = new QTableWidgetItem(*m_emptyCell); lbl->setText("Gradient");
  m_uiForm.tbDerived->setItem(0, SANSPlotSpecial::FitInformation, lbl);
  m_uiForm.tbDerived->setItem(0, SANSPlotSpecial::FitInformationValues, m_derivatives["Gradient"]);
  lbl = new QTableWidgetItem(*m_emptyCell); lbl->setText("Intercept");
  m_uiForm.tbDerived->setItem(1, SANSPlotSpecial::FitInformation, lbl);
  m_uiForm.tbDerived->setItem(1, SANSPlotSpecial::FitInformationValues, m_derivatives["Intercept"]);
  lbl = new QTableWidgetItem(*m_emptyCell); lbl->setText("Chi Squared");
  m_uiForm.tbDerived->setItem(2, SANSPlotSpecial::FitInformation, lbl);
  m_uiForm.tbDerived->setItem(2, SANSPlotSpecial::FitInformationValues, m_derivatives["Chi Squared"]);
    
  m_derivatives["Rg"] = new QTableWidgetItem();
  m_derivatives["Rg"]->setToolTip("Radius of gyration");
  m_units["Rg"] = QString::fromUtf8("\xc3\x85");
  m_derivatives["Rg,xs"] = new QTableWidgetItem(*m_emptyCell);
  m_derivatives["Rg,xs"]->setToolTip("Cross-sectional radius of gyration");
  m_units["Rg,xs"] = QString::fromUtf8("\xc3\x85");
  m_derivatives["R"] = new QTableWidgetItem(*m_emptyCell);
  m_derivatives["R"]->setToolTip("Equivalent spherical radius");
  m_units["R"] = QString::fromUtf8("\xc3\x85");
  m_derivatives["T"] = new QTableWidgetItem(*m_emptyCell);
  m_derivatives["T"]->setToolTip("Thickness");
  m_units["T"] = QString::fromUtf8("\xc3\x85");
  m_derivatives["C"] = new QTableWidgetItem();
  m_derivatives["C"]->setToolTip("Concentration");
  m_units["C"] = "g/cm^3";
  m_derivatives["Phi"] = new QTableWidgetItem();
  m_derivatives["Phi"]->setToolTip("Volume fraction");
  m_units["Phi"] = "%/100";
  m_derivatives["Deltarho"] = new QTableWidgetItem();
  m_derivatives["Deltarho"]->setToolTip("Difference in neutron scattering length densities (solute-solvent)");
  m_units["Deltarho"] = "cm^-2";
  m_derivatives["M"] = new QTableWidgetItem();
  m_derivatives["M"]->setToolTip("Molecular weight");
  m_units["M"] = "g/mol";
  m_derivatives["ML"] = new QTableWidgetItem();
  m_derivatives["ML"]->setToolTip("Mass per unit length");
  m_units["ML"] = "g/mol per segment";
  m_derivatives["D"] = new QTableWidgetItem();
  m_derivatives["D"]->setToolTip("Bulk density");
  m_units["D"] = "g/cm^3";
  m_derivatives["N"] = new QTableWidgetItem(*m_emptyCell);
  m_derivatives["N"]->setToolTip("Q-Dependence");
  m_units["N"] = "(unitless)";
  m_derivatives["V"] = new QTableWidgetItem(*m_emptyCell);
  m_derivatives["V"]->setToolTip("Excluded volume component");
  m_units["V"] = "(unitless)";
  m_derivatives["Zeta"] = new QTableWidgetItem(*m_emptyCell);
  m_derivatives["Zeta"]->setToolTip("Characteristic length");
  m_units["Zeta"] = QString::fromUtf8("\xc3\x85");
  m_derivatives["(S/V)"] = new QTableWidgetItem();
  m_derivatives["(S/V)"]->setToolTip("Surface area-to-volume ratio");
  m_units["(S/V)"] = "cm^-1";
}

QwtPlotCurve* SANSPlotSpecial::plotMiniplot(QwtPlotCurve* curve, boost::shared_ptr<Mantid::API::MatrixWorkspace> workspace,
  size_t workspaceIndex)
{
  bool data = ( curve == m_dataCurve );

  if ( curve != NULL )
  {
    curve->attach(0);
    delete curve;
    curve = 0;
  }

  curve = new QwtPlotCurve();

  using Mantid::MantidVec;
  const MantidVec & dataX = workspace->readX(workspaceIndex);
  const MantidVec & dataY = workspace->readY(workspaceIndex);

  curve->setData(&dataX[0], &dataY[0], static_cast<int>(workspace->blocksize()));
  curve->attach(m_uiForm.plotWindow);

  m_uiForm.plotWindow->replot();
  
  if ( data )
  {
    m_rangeSelector->setRange(dataX.front(), dataX.back());
  }

  return curve;
}

void SANSPlotSpecial::deriveGuinierSpheres()
{
  // Intercept = M.[(c.(deltarho**2) / (NA.d**2)] = M.[(phi.(deltarho**2) / (NA.d)]
  QPair<QStringList, QMap<QString, double> > props = getProperties("Guinier (spheres)");
  QStringList unknown = props.first;
  QMap<QString, double> values = props.second;

  bool doable = ( unknown.indexOf("C") != -1 ) ^ ( unknown.indexOf("Phi") != -1 );

  if ( ( ( unknown.size() > 1 ) && ! doable ) || ( unknown.size() > 2 ) )
  {
    return;
  }
  QString route = "Phi";
  if ( unknown.indexOf("C") == -1 ) { route = "C"; }

  const double lhs = values["Intercept"] * Mantid::PhysicalConstants::N_A;

  foreach ( QString item, unknown )
  {
    double val;
    if ( item == "M" )
    {
      val = lhs * values["D"] / std::pow(values["Deltarho"], 2.0);
      if ( route == "C" )
      {
        val = val * ( values["D"] / values["C"] );
      }
      else
      {
        val = val / values["Phi"];
      }
    }
    else if ( item == "C" )
    {
      val = ( lhs * std::pow(values["D"], 2.0) ) / ( values["M"] * std::pow(values["Deltarho"], 2.0) );
    }
    else if ( item == "Deltarho" )
    {
      val = lhs * values["D"] / values["M"];
      if ( route == "C" )
      {
        val = val * values["D"] / values["C"];
      }
      else
      {
        val = val / values["Phi"];
      }
      val = std::sqrt( val );
    }
    else if ( item == "D" )
    {
      val = lhs / ( values["M"] * std::pow(values["Deltarho"], 2.0) );
      if ( route == "C" )
      {
        val = 1 / ( std::sqrt( val / values["C"] ) );
      }
      else
      {
        val = 1 / ( val / values["Phi"] );
      }
    }
    else if ( item == "Phi" )
    {
      val = ( lhs * values["D"] ) / ( values["M"] * std::pow(values["Deltarho"], 2.0) );
    }
    else { continue; }

    values[item] = val;
    m_derivatives[item]->setText(QString::number(val));
  }
}

void SANSPlotSpecial::deriveGuinierRods()
{
  //Intercept (Q**2=0) = Ln[(pi.c.(deltarho**2).ML) / (NA.d**2)]
  QPair<QStringList, QMap<QString, double> > props = getProperties("Guinier (rods)");
  QStringList unknown = props.first;
  QMap<QString, double> values = props.second;

  if ( unknown.size() != 1 )
  {
    return;
  }

  QString item = unknown.front();

  double val;

  const double lhs = ( std::pow(2.71828183, values["Intercept"]) * Mantid::PhysicalConstants::N_A ) / M_PI;
  
  if ( item == "C" )
  {
    val = lhs * ( std::pow(values["D"], 2) / ( std::pow(values["Deltarho"], 2) * values["ML"] ) );
  }
  else if ( item == "Deltarho" )
  {
    val = std::sqrt( lhs * ( std::pow(values["D"], 2) / ( values["C"] * values["ML"] ) ) );
  }
  else if ( item == "ML" )
  {
    val = lhs * ( std::pow(values["D"], 2) / ( std::pow(values["Deltarho"], 2) * values["C"] ) );
  }
  else if ( item == "D" )
  {
    val = std::sqrt( 1 / ( lhs / ( values["C"] * values["ML"] * std::pow(values["Deltarho"], 2) ) ) );
  }
  else { return; }

  m_derivatives[item]->setText(QString::number(val));

}

void SANSPlotSpecial::deriveZimm()
{
  // Intercept = (1/M).[(NA.d**2) / (c.(deltarho**2)] = (1/M).[(NA.d) / (phi.(deltarho**2)]
  QPair<QStringList, QMap<QString, double> > props = getProperties("Zimm");
  QStringList unknown = props.first;
  QMap<QString, double> values = props.second;

  const double lhs = values["Intercept"] / Mantid::PhysicalConstants::N_A;

  bool doable = ( unknown.indexOf("C") != -1 ) ^ ( unknown.indexOf("Phi") != -1 );
  if ( ( ( unknown.size() > 1 ) && ! doable ) || ( unknown.size() > 2 ) )
  {
    return;
  }
  QString route = "Phi";
  if ( unknown.indexOf("C") == -1 ) { route = "C"; }


  foreach ( QString item, unknown )
  {
    double val;
    if ( item == "D" )
    {
      val = lhs * values["M"] * std::pow(values["Deltarho"], 2);
      if ( route == "C" )
      {
        val = std::sqrt( val * values["C"] );
      }
      else
      {
        val = val * values["Phi"];
      }
    }
    else if ( item == "M" )
    {
      val = lhs * std::pow(values["Deltarho"], 2.0) / values["D"];
      if ( route == "C" )
      {
        val = 1 / ( val * values["C"] / values["D"] );
      }
      else
      {
        val = 1 / ( val * values["Phi"] );
      }
    }
    else if ( item == "C" )
    {
      val = 1 / ( lhs * ( values["M"] * std::pow(values["Deltarho"], 2) ) / std::pow(values["D"], 2) );
    }
    else if ( item == "Deltarho" )
    {
      val = lhs * values["M"] / values["D"];
      if ( route == "C" )
      {
        val = val * values["C"] / values["D"];
      }
      else
      {
        val = val * values["Phi"];
      }
      val = std::sqrt( 1 / val ) ;
    }
    else if ( item == "Phi" )
    {
      val = lhs * ( values["M"] * std::pow(values["Deltarho"], 2) ) / values["D"];
    }
    else { continue; }
    
    values[item] = val;
    m_derivatives[item]->setText(QString::number(val));
  }
}

void SANSPlotSpecial::deriveKratky()
{
  // Plateau Intercept = [(2.c.M.(deltarho**2)) / (NA.(d**2).(Rg**2))] = [(2.phi.M.(deltarho**2)) / (NA.d.(Rg**2))]
  QPair<QStringList, QMap<QString, double> > props = getProperties("Kratky");
  QStringList unknown = props.first;
  QMap<QString, double> values = props.second;

  const double lhs = Mantid::PhysicalConstants::N_A * values["Intercept"] / 2.0;

  bool doable = ( unknown.indexOf("C") != -1 ) ^ ( unknown.indexOf("Phi") != -1 );
  if ( ( ( unknown.size() > 1 ) && ! doable ) || ( unknown.size() > 2 ) )
  {
    return;
  }
  QString route = "Phi";
  if ( unknown.indexOf("C") == -1 ) { route = "C"; }
  
  foreach ( QString item, unknown )
  {
    double val;

    if ( item == "C" )
    {
      val = lhs * ( std::pow(values["D"], 2) * std::pow(values["Rg"], 2) ) / ( values["M"] * std::pow(values["Deltarho"], 2) );
    }
    else if ( item == "M" )
    {
      val = lhs * ( values["D"] * std::pow(values["Rg"], 2) ) / std::pow(values["Deltarho"], 2);
      if ( route == "C" )
      {
        val = val * ( values["D"] / values["C"] );
      }
      else
      {
        val = val / values["Phi"];
      }
    }
    else if ( item == "Deltarho" )
    {
      val = lhs * ( values["D"] * std::pow(values["Rg"], 2) ) / values["M"];
      if ( route == "C" )
      {
        val = val * ( values["D"] / values["C"] );
      }
      else
      {
        val = val / values["Phi"];
      }
      val = std::sqrt( val );
    }
    else if ( item == "D" )
    {
      val = lhs * std::pow(values["Rg"], 2) / ( values["M"] * std::pow(values["Deltarho"], 2 ) );
      if ( route == "C" )
      {
        val = std::sqrt( 1 / ( val / values["C"] ) );
      }
      else
      {
        val = 1 / ( val / values["Phi"] );
      }
    }
    else if ( item == "Rg" )
    {
      val = lhs * values["D"] / ( values["M"] * std::pow(values["Deltarho"], 2 ) );
      if ( route == "C" )
      {
        val = val * values["D"] / values["C"];
      }
      else
      {
        val = val / values["Phi"];
      }
      val = std::sqrt( 1 / val );
    }
    else if ( item == "Phi" )
    {
      val = lhs * ( values["D"] * std::pow(values["Rg"], 2) ) / ( values["M"] * std::pow(values["Deltarho"], 2) );
    }
    else { continue; }

    values[item] = val;
    m_derivatives[item]->setText(QString::number(val));
  }
}

void SANSPlotSpecial::derivePorod()
{
  // Plateau Intercept = [(2.pi.c.(deltarho**2)) / d].(S / V)
  QPair<QStringList, QMap<QString, double> > props = getProperties("Porod");
  QStringList unknown = props.first;
  QMap<QString, double> values = props.second;

  if ( unknown.size() != 1 ) { return; }

  QString item = unknown.front();
  double val;
  double lhs = values["Intercept"] / ( 2 * M_PI );

  if ( item == "C" )
  {
    val = ( lhs * values["D"] ) / ( std::pow(values["Deltarho"], 2) * values["(S/V)"] );
  }
  else if ( item == "Deltarho" )
  {
    val = std::sqrt( ( lhs * values["D"] ) / ( values["C"] * values["(S/V)"] ) );
  }
  else if ( item == "(S/V)" )
  {
    val = ( lhs * values["D"] ) / ( std::pow(values["Deltarho"], 2) * values["C"] );
  }
  else if ( item == "D" )
  {
    val =  1 / ( lhs / ( values["C"] * std::pow(values["Deltarho"], 2) * values["(S/V)"] ) );
  }
  else { return; }

  m_derivatives[item]->setText(QString::number(val));
}

double SANSPlotSpecial::getValue(QTableWidgetItem* item)
{
  QString text = item->text().trimmed();
  if ( text == "nan" ) { item->setText(""); }

  bool ok(false);
  const double result = item->text().trimmed().toDouble(&ok);

  if ( text.isEmpty() || !ok )
  {
    throw std::invalid_argument("Could not convert value given to a double.");
  }

  return result;
}

QPair<QStringList, QMap<QString, double> > SANSPlotSpecial::getProperties(const QString & transform)
{
  QStringList items = m_transforms[transform]->interceptDerivatives();
  items << "Intercept";
  QMap<QString, double> values;
  QStringList unknown;
  foreach ( QString item, items )
  {
    try
    {
      double val = getValue(m_derivatives[item]);
      values[item] = val;
    } catch ( std::invalid_argument & )
    {
      unknown << item;
    }
  }

  QPair<QStringList, QMap<QString, double> > result;
  result.first = unknown;
  result.second = values;

  return result;
}

//--------------------------------------------------------------------
//------- Utility "Transform" Class ----------------------------------
//--------------------------------------------------------------------
SANSPlotSpecial::Transform::Transform(Transform::TransformType type) : m_type(type), 
  m_xWidgets(QList<QWidget*>()), m_yWidgets(QList<QWidget*>()), m_gDeriv(""), m_iDeriv("")
{
  init();
}

SANSPlotSpecial::Transform::~Transform() {}

void SANSPlotSpecial::Transform::init()
{
  m_xWidgets.clear();
  m_yWidgets.clear();

  switch ( m_type )
  {
  case GuinierSpheres:
    m_xWidgets.append(new QLabel("Q^2"));
    m_yWidgets.append(new QLabel("ln (I)"));
    m_gDeriv = "Rg|R";
    m_iDeriv = "M|C|Deltarho|D|Phi";
    break;
  case GuinierRods:
    m_xWidgets.append(new QLabel("Q^2"));
    m_yWidgets.append(new QLabel("ln (I (Q) )"));
    m_gDeriv = "Rg,xs";
    m_iDeriv = "C|Deltarho|ML|D";
    break;
  case GuinierSheets:
    m_xWidgets.append(new QLabel("Q^2"));
    m_yWidgets.append(new QLabel("ln (I (Q ^ 2 ) )"));
    m_gDeriv = "T";
    break;
  case Zimm:
    m_xWidgets.append(new QLabel("Q^2"));
    m_yWidgets.append(new QLabel("1 / I"));
    m_gDeriv = "Rg|R";
    m_iDeriv = "M|D|C|Deltarho|Phi";
    break;
  case DebyeBueche:
    m_xWidgets.append(new QLabel("Q^2"));
    m_yWidgets.append(new QLabel("1 / sqrt (I)"));
    m_gDeriv = "Zeta"; // Weird ? Zeta = sqrt( graident / intercept )
    break;
  case Holtzer:
    m_xWidgets.append(new QLabel("Q"));
    m_yWidgets.append(new QLabel("I * Q"));
    break;
  case Kratky:
    m_xWidgets.append(new QLabel("Q"));
    m_yWidgets.append(new QLabel("I * Q^2"));
    m_iDeriv = "C|M|Deltarho|D|Rg|Phi";
    break;
  case Porod:
    m_xWidgets.append(new QLabel("Q"));
    m_yWidgets.append(new QLabel("I * Q^4"));
    m_iDeriv = "C|Deltarho|D|(S/V)";
    break;
  case LogLog:
    m_xWidgets.append(new QLabel("ln (Q)"));
    m_yWidgets.append(new QLabel("ln (I)"));
    m_gDeriv = "N|V";
    break;
  case General:
    m_xWidgets.append(new QLabel("Q^"));
    m_xWidgets.append(new QLineEdit("C6"));
    m_xWidgets.append(new QLabel("*I^"));
    m_xWidgets.append(new QLineEdit("C7"));
    m_xWidgets.append(new QLabel("*ln(Q^"));
    m_xWidgets.append(new QLineEdit("C8"));
    m_xWidgets.append(new QLabel("*I^"));
    m_xWidgets.append(new QLineEdit("C9"));
    m_xWidgets.append(new QLabel("*"));
    m_xWidgets.append(new QLineEdit("C10"));
    m_xWidgets.append(new QLabel(")"));
    m_yWidgets.append(new QLabel("Q^"));
    m_yWidgets.append(new QLineEdit("C1"));
    m_yWidgets.append(new QLabel("*I^"));
    m_yWidgets.append(new QLineEdit("C2"));
    m_yWidgets.append(new QLabel("*ln(Q^"));
    m_yWidgets.append(new QLineEdit("C3"));
    m_yWidgets.append(new QLabel("*I^"));
    m_yWidgets.append(new QLineEdit("C4"));
    m_yWidgets.append(new QLabel("*"));
    m_yWidgets.append(new QLineEdit("C5"));
    m_yWidgets.append(new QLabel(")"));
    tidyGeneral();
    break;
  }

}

std::vector<double> SANSPlotSpecial::Transform::functionConstants()
{
  std::vector<double> result;
  if ( m_type != General ) { return result; }

  foreach ( QWidget* item, m_yWidgets )
  {
    if ( item->isA("QLineEdit") )
    {
      item->setMaximumSize(25,20);
      QString le = dynamic_cast<QLineEdit*>(item)->text();
      result.push_back(le.toDouble());
    }
  }

  foreach ( QWidget* item, m_xWidgets )
  {
    if ( item->isA("QLineEdit") )
    {
      item->setMaximumSize(25,20);
      QString le = dynamic_cast<QLineEdit*>(item)->text();
      result.push_back(le.toDouble());
    }
  }
  return result;
}

QPair<QStringList, QList<QPair<int, int> > > SANSPlotSpecial::Transform::derivatives()
{
  QStringList dg = m_gDeriv.split("|", QString::SkipEmptyParts);
  QStringList di = m_iDeriv.split("|", QString::SkipEmptyParts);
  QStringList items = dg + di;

  QList<QPair<int, int> > positions;

  for ( int i = 0; i < dg.size(); i++ )
  {
    positions.append(QPair<int, int>(i, SANSPlotSpecial::GradientLabels));
  }

  for ( int i = 0; i < di.size(); i++ )
  {
    positions.append(QPair<int, int>(i, SANSPlotSpecial::InterceptLabels));
  }

  QPair<QStringList, QList<QPair<int, int> > > result = QPair<QStringList, QList<QPair<int, int> > >(items, positions);
  return result;
}

QStringList SANSPlotSpecial::Transform::interceptDerivatives()
{
  QStringList result = m_iDeriv.split("|", QString::SkipEmptyParts);
  return result;
}

void SANSPlotSpecial::Transform::tidyGeneral()
{
  foreach ( QWidget* item, m_xWidgets )
  {
    item->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    if ( item->isA("QLineEdit") )
    {
      item->setMaximumSize(25,20);
    }
  }

  foreach ( QWidget* item, m_yWidgets )
  {
    item->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    if ( item->isA("QLineEdit") )
    {
      item->setMaximumSize(25,20);
    }
  }
}

}
}

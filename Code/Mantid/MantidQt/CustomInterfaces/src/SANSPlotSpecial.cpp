#include "MantidQtCustomInterfaces/SANSPlotSpecial.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"

#include "MantidQtMantidWidgets/RangeSelector.h"

#include <QLineEdit>
// #include <math>
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
  Mantid::API::IAlgorithm_sptr fit = Mantid::API::AlgorithmManager::Instance().create("Linear");
  fit->initialize();
  fit->setProperty<Mantid::API::MatrixWorkspace_sptr>("InputWorkspace", m_workspaceIQT);
  fit->setPropertyValue("OutputWorkspace", "__sans_isis_display_linear");
  fit->setProperty<double>("StartX", low);
  fit->setProperty<double>("EndX", high);
  fit->execute();

  m_workspaceLinear = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
    (Mantid::API::AnalysisDataService::Instance().retrieve("__sans_isis_display_linear"));
  m_linearCurve = plotMiniplot(m_linearCurve, m_workspaceLinear);

  QPen fitPen(Qt::red, Qt::SolidLine);
  m_linearCurve->setPen(fitPen);
  m_uiForm.plotWindow->replot();

  double intercept = fit->getProperty("FitIntercept");
  double gradient = fit->getProperty("FitSlope");
  double chisqrd = fit->getProperty("Chi2");

  m_derivatives["Intercept"]->setText(QString::number(intercept));
  m_derivatives["Gradient"]->setText(QString::number(gradient));
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
  // plot data to the plotWindow
  m_dataCurve = plotMiniplot(m_dataCurve, m_workspaceIQT);
  // update fields of table of "derived" values?
  QPair<QStringList, QList<QPair<int, int> > > deriv = m_transforms[m_uiForm.cbPlotType->currentText()]->derivatives();
  tableDisplay(deriv.first, deriv.second);
  calculateDerivatives();
}

void SANSPlotSpecial::help()
{
  //
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
    m_uiForm.tbDerived->setItem(i, 2, new QTableWidgetItem(*m_emptyCell));
    m_uiForm.tbDerived->setItem(i, 4, new QTableWidgetItem(*m_emptyCell));
    m_uiForm.tbDerived->takeItem(i, 3);
    m_uiForm.tbDerived->takeItem(i, 5);
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
    temp = - std::sqrt(3 * gradient);
    m_derivatives["Rg"]->setText(QString::number(temp));
    temp = - std::sqrt(5 * gradient);
    m_derivatives["R"]->setText(QString::number(temp));
    // Intercept = M.[(c.(deltarho**2) / (NA.d**2)] = M.[(phi.(deltarho**2) / (NA.d)]
    break;
  case Transform::GuinierRods:
    // Gradient = -(Rg,xs**2)/2  (note dividing by 2 this time)
    temp = - std::sqrt(2 * gradient);
    m_derivatives["Rg,xs"]->setText(QString::number(temp));
    //Intercept (Q**2=0) = Ln[(pi.c.(deltarho**2).ML) / (NA.d**2)] 
    break;
  case Transform::GuinierSheets:
    temp = - std::sqrt(gradient * 12);
    m_derivatives["T"]->setText(QString::number(temp));
    break;
  case Transform::Zimm:
    // Gradient = (Rg**2)/3 = (R**2)/5
    temp = std::sqrt(3 * gradient);
    m_derivatives["Rg"]->setText(QString::number(temp));
    temp = std::sqrt(5 * gradient);
    m_derivatives["R"]->setText(QString::number(temp));
    // Intercept = (1/M).[(NA.d**2) / (c.(deltarho**2)] = (1/M).[(NA.d) / (phi.(deltarho**2)]
    break;
  case Transform::Kratky:
    // Plateau Intercept = [(2.c.M.(deltarho**2)) / (NA.(d**2).(Rg**2))] = [(2.phi.M.(deltarho**2)) / (NA.d.(Rg**2))]
    break;
  case Transform::DebyeBueche:
    temp = std::sqrt( gradient / intercept );
    m_derivatives["Zeta"]->setText(QString::number(temp));
    break;
  case Transform::LogLog:
    temp = - gradient;
    m_derivatives["N"]->setText(QString::number(temp));
    temp = 1 / gradient;
    m_derivatives["V"]->setText(QString::number(temp));
    break;
  case Transform::Porod:
    // Plateau Intercept = [(2.pi.c.(deltarho**2)) / d].(S / V)
    break;
  default:
    break;
  }

  m_rearrangingTable = false;
}

void SANSPlotSpecial::tableUpdated(int row, int column)
{
  if ( m_rearrangingTable ) { return; }
  if ( ! ( column == 3 || column == 5 ) ) { return; }

  // Transform::TransformType type = m_transforms[m_uiForm.cbPlotType->currentText()]->type();

  calculateDerivatives();
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

  // Other signal/slot connections
  connect(m_uiForm.pbPlot, SIGNAL(clicked()), this, SLOT(plot()));
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(help()));
  connect(m_uiForm.cbBackground, SIGNAL(currentIndexChanged(int)), m_uiForm.swBackground, SLOT(setCurrentIndex(int)));
  connect(m_uiForm.cbPlotType, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(updateAxisLabels(const QString &)));
  connect(m_uiForm.tbDerived, SIGNAL(cellChanged(int, int)), this, SLOT(tableUpdated(int, int)));

  updateAxisLabels(m_uiForm.cbPlotType->currentText());
}

Mantid::API::MatrixWorkspace_sptr SANSPlotSpecial::runIQTransform()
{
  // Run the IQTransform algorithm for the current settings on the GUI
  Mantid::API::IAlgorithm_sptr iqt = Mantid::API::AlgorithmManager::Instance().create("IQTransform");
  iqt->initialize();
  iqt->setPropertyValue("InputWorkspace", m_uiForm.wsInput->currentText().toStdString());
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
    lblItm->setText((*it));
    m_uiForm.tbDerived->setItem(row, column, lblItm);
    m_uiForm.tbDerived->setItem(row, column+1, m_derivatives[(*it)]);
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
{  // Setup the table
  m_emptyCell = new QTableWidgetItem();
  m_emptyCell->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled );
  m_uiForm.tbDerived->setItemPrototype(m_emptyCell);

  m_derivatives["Gradient"] = new QTableWidgetItem(*m_emptyCell);
  m_derivatives["Intercept"] = new QTableWidgetItem(*m_emptyCell);
  m_derivatives["Chi Squared"] = new QTableWidgetItem(*m_emptyCell);

  QTableWidgetItem* lbl = new QTableWidgetItem(*m_emptyCell); lbl->setText("Gradient");
  m_uiForm.tbDerived->setItem(0,0, lbl);
  m_uiForm.tbDerived->setItem(0,1, m_derivatives["Gradient"]);
  lbl = new QTableWidgetItem(*m_emptyCell); lbl->setText("Intercept");
  m_uiForm.tbDerived->setItem(1,0, lbl);
  m_uiForm.tbDerived->setItem(1,1, m_derivatives["Intercept"]);
  lbl = new QTableWidgetItem(*m_emptyCell); lbl->setText("Chi Squared");
  m_uiForm.tbDerived->setItem(2,0, lbl);
  m_uiForm.tbDerived->setItem(2,1, m_derivatives["Chi Squared"]);

  // Properties that can not be changed should be cloned from m_emptyCell
  
  m_derivatives["Rg"] = new QTableWidgetItem(); // Radius of gyration
  m_derivatives["Rg,xs"] = new QTableWidgetItem(); // Cross-sectional radius of gyration
  m_derivatives["R"] = new QTableWidgetItem(); // Equivalent spherical radius
  m_derivatives["T"] = new QTableWidgetItem(*m_emptyCell); // Thickness
  m_derivatives["C"] = new QTableWidgetItem(); // Concentration
  m_derivatives["Phi"] = new QTableWidgetItem(); // Volume fraction
  m_derivatives["Deltarho"] = new QTableWidgetItem(); // Difference in neutron scattering length densities (solute-solvent)
  m_derivatives["M"] = new QTableWidgetItem(); // Molecular weight
  m_derivatives["ML"] = new QTableWidgetItem(); // Mass per unit length
  m_derivatives["D"] = new QTableWidgetItem(); // Bulk density
  m_derivatives["N"] = new QTableWidgetItem(*m_emptyCell); // Q-Dependence
  m_derivatives["V"] = new QTableWidgetItem(*m_emptyCell); // Excluded volume component
  m_derivatives["Zeta"] = new QTableWidgetItem(*m_emptyCell); // Characteristic length
  m_derivatives["(S/V)"] = new QTableWidgetItem(); // Survace area-to-volume ratio
}

QwtPlotCurve* SANSPlotSpecial::plotMiniplot(QwtPlotCurve* curve, Mantid::API::MatrixWorkspace_sptr workspace)
{
  bool data = ( curve == m_dataCurve );

  if ( curve != NULL )
  {
    curve->attach(0);
    delete curve;
    curve = 0;
  }

  curve = new QwtPlotCurve();

  const QVector<double> dataX = QVector<double>::fromStdVector(workspace->readX(0));
  const QVector<double> dataY = QVector<double>::fromStdVector(workspace->readY(0));

  curve->setData(dataX, dataY);
  curve->attach(m_uiForm.plotWindow);

  m_uiForm.plotWindow->replot();
  
  if ( data )
  {
    double low = dataX.first();
    double high = dataX.last();
    m_rangeSelector->setRange(low, high);
  }

  return curve;
}

//--------------------------------------------------------------------
//------- Utility "Transform" Class ----------------------------------
//--------------------------------------------------------------------
SANSPlotSpecial::Transform::Transform(Transform::TransformType type, QWidget* parent) : m_type(type), 
  m_xWidgets(QList<QWidget*>()), m_yWidgets(QList<QWidget*>()), m_parent(parent), m_gDeriv(""), m_iDeriv("")
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
    m_xWidgets.append(new QLabel("Q^2", this));
    m_yWidgets.append(new QLabel("ln (I)", this));
    m_gDeriv = "Rg|R";
    m_iDeriv = "M|C|Deltarho|D|Phi";
    break;
  case GuinierRods:
    m_xWidgets.append(new QLabel("Q^2", this));
    m_yWidgets.append(new QLabel("ln (I (Q) )", this));
    m_gDeriv = "Rg,xs";
    m_iDeriv = "C|Deltarho|ML|D";
    break;
  case GuinierSheets:
    m_xWidgets.append(new QLabel("Q^2", this));
    m_yWidgets.append(new QLabel("ln (I (Q ^ 2 ) )", this));
    m_gDeriv = "T";
    break;
  case Zimm:
    m_xWidgets.append(new QLabel("Q^2", this));
    m_yWidgets.append(new QLabel("1 / I", this));
    m_gDeriv = "Rg|R";
    m_iDeriv = "M|D|C|Deltarho|Phi";
    break;
  case DebyeBueche:
    m_xWidgets.append(new QLabel("Q^2", this));
    m_yWidgets.append(new QLabel("1 / sqrt (I)", this));
    m_gDeriv = "Zeta"; // Weird ? Zeta = sqrt( graident / intercept )
    break;
  case Holtzer:
    m_xWidgets.append(new QLabel("Q", this));
    m_yWidgets.append(new QLabel("I * Q", this));
    break;
  case Kratky:
    m_xWidgets.append(new QLabel("Q", this));
    m_yWidgets.append(new QLabel("I * Q^2", this));
    m_iDeriv = "C|M|Deltarho|D|Rg|Phi";
    break;
  case Porod:
    m_xWidgets.append(new QLabel("Q", this));
    m_yWidgets.append(new QLabel("I * Q^4", this));
    m_iDeriv = "C|Deltarho|D|S|V";
    break;
  case LogLog:
    m_xWidgets.append(new QLabel("ln (Q)", this));
    m_yWidgets.append(new QLabel("ln (I)", this));
    m_gDeriv = "N|V";
    break;
  case General:
    m_xWidgets.append(new QLabel("Q^", this));
    m_xWidgets.append(new QLineEdit("C6", this));
    m_xWidgets.append(new QLabel("*I^", this));
    m_xWidgets.append(new QLineEdit("C7", this));
    m_xWidgets.append(new QLabel("*ln(Q^", this));
    m_xWidgets.append(new QLineEdit("C8", this));
    m_xWidgets.append(new QLabel("*I^", this));
    m_xWidgets.append(new QLineEdit("C9", this));
    m_xWidgets.append(new QLabel("*", this));
    m_xWidgets.append(new QLineEdit("C10", this));
    m_xWidgets.append(new QLabel(")", this));
    m_yWidgets.append(new QLabel("Q^", this));
    m_yWidgets.append(new QLineEdit("C1", this));
    m_yWidgets.append(new QLabel("*I^", this));
    m_yWidgets.append(new QLineEdit("C2", this));
    m_yWidgets.append(new QLabel("*ln(Q^", this));
    m_yWidgets.append(new QLineEdit("C3", this));
    m_yWidgets.append(new QLabel("*I^", this));
    m_yWidgets.append(new QLineEdit("C4", this));
    m_yWidgets.append(new QLabel("*", this));
    m_yWidgets.append(new QLineEdit("C5", this));
    m_yWidgets.append(new QLabel(")", this));
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
    positions.append(QPair<int, int>(i,2));
  }

  for ( int i = 0; i < di.size(); i++ )
  {
    positions.append(QPair<int, int>(i,4));
  }

  QPair<QStringList, QList<QPair<int, int> > > result = QPair<QStringList, QList<QPair<int, int> > >(items, positions);
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

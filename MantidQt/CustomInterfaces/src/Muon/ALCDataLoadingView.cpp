#include "MantidQtCustomInterfaces/Muon/ALCDataLoadingView.h"

#include "MantidQtAPI/HelpWindow.h"
#include "MantidQtMantidWidgets/ErrorCurve.h"

#include <QMessageBox>

#include <qwt_symbol.h>

namespace MantidQt
{
namespace CustomInterfaces
{
  ALCDataLoadingView::ALCDataLoadingView(QWidget* widget)
    : m_widget(widget), m_dataCurve(new QwtPlotCurve()), m_dataErrorCurve(NULL)
  {}

  ALCDataLoadingView::~ALCDataLoadingView() {
    m_dataCurve->detach();
    delete m_dataCurve;
    if (m_dataErrorCurve) {
      m_dataErrorCurve->detach();
      delete m_dataErrorCurve;
    }
  }

  void ALCDataLoadingView::initialize()
  {
    m_ui.setupUi(m_widget);
    connect(m_ui.load, SIGNAL(clicked()), SIGNAL(loadRequested()));
    connect(m_ui.firstRun, SIGNAL(fileFindingFinished()), SIGNAL(firstRunSelected()));

    connect(m_ui.help, SIGNAL(clicked()), this, SLOT(help()));

    m_ui.dataPlot->setCanvasBackground(Qt::white);
    m_ui.dataPlot->setAxisFont(QwtPlot::xBottom, m_widget->font());
    m_ui.dataPlot->setAxisFont(QwtPlot::yLeft, m_widget->font());

    m_dataCurve->setStyle(QwtPlotCurve::NoCurve);
    m_dataCurve->setSymbol(QwtSymbol(QwtSymbol::Ellipse, QBrush(), QPen(), QSize(7,7)));
    m_dataCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    m_dataCurve->attach(m_ui.dataPlot);

    // The following lines disable the groups' titles when the
    // group is disabled
    QPalette palette;
    palette.setColor(QPalette::Disabled, QPalette::WindowText,
                     QApplication::palette().color(QPalette::Disabled,
                                                   QPalette::WindowText));
    m_ui.dataGroup->setPalette(palette);
    m_ui.deadTimeGroup->setPalette(palette);
    m_ui.detectorGroupingGroup->setPalette(palette);
    m_ui.periodsGroup->setPalette(palette);
    m_ui.calculationGroup->setPalette(palette);
  }

  std::string ALCDataLoadingView::firstRun() const
  {
    if (m_ui.firstRun->isValid())
    {
      return m_ui.firstRun->getFirstFilename().toStdString();
    }
    else
    {
      return "";
    }
  }

  std::string ALCDataLoadingView::lastRun() const
  {
    if (m_ui.lastRun->isValid())
    {
      return m_ui.lastRun->getFirstFilename().toStdString();
    }
    else
    {
      return "";
    }
  }

  std::string ALCDataLoadingView::log() const
  {
    return m_ui.log->currentText().toStdString();
  }

  std::string ALCDataLoadingView::function() const
  {
    return m_ui.function->currentText().toStdString();
  }

  std::string ALCDataLoadingView::calculationType() const
  {
    // XXX: "text" property of the buttons should be set correctly, as accepted by
    //      PlotAsymmetryByLogValue
    return m_ui.calculationType->checkedButton()->text().toStdString();
  }

  std::string ALCDataLoadingView::deadTimeType() const
  {
    std::string checkedButton = m_ui.deadTimeCorrType->checkedButton()->text().toStdString();
    if ( checkedButton == "From Data File" ) {
        return std::string("FromRunData");
    } else if ( checkedButton == "From Custom File" ) {
      return std::string("FromSpecifiedFile");
    } else {
      return checkedButton;
    }
  }

  std::string ALCDataLoadingView::deadTimeFile() const
  {
    if (deadTimeType()=="FromSpecifiedFile") {
      return m_ui.deadTimeFile->getFirstFilename().toStdString();
    } else {
      return "";
    }
  }

  std::string ALCDataLoadingView::detectorGroupingType() const
  {
    std::string checkedButton = m_ui.detectorGroupingType->checkedButton()->text().toStdString();
    return checkedButton;
  }

  std::string ALCDataLoadingView::getForwardGrouping() const
  {
    return m_ui.forwardEdit->text().toStdString();
  }

  std::string ALCDataLoadingView::getBackwardGrouping() const
  {
    return m_ui.backwardEdit->text().toStdString();
  }

  std::string ALCDataLoadingView::redPeriod() const
  {
    return m_ui.redPeriod->currentText().toStdString();
  }

  std::string ALCDataLoadingView::greenPeriod() const
  {
    return m_ui.greenPeriod->currentText().toStdString();
  }

  bool ALCDataLoadingView::subtractIsChecked() const
  {
    return m_ui.subtractCheckbox->isChecked();
  }

  boost::optional< std::pair<double,double> > ALCDataLoadingView::timeRange() const
  {
    auto range = std::make_pair(m_ui.minTime->value(), m_ui.maxTime->value());
    return boost::make_optional(range);
  }

  void ALCDataLoadingView::setDataCurve(const QwtData &data,
                                        const std::vector<double> &errors) {

    // Set data
    m_dataCurve->setData(data);

    // Set errors
    if (m_dataErrorCurve) {
      m_dataErrorCurve->detach();
      delete m_dataErrorCurve;
    }
    m_dataErrorCurve =
        new MantidQt::MantidWidgets::ErrorCurve(m_dataCurve, errors);
    m_dataErrorCurve->attach(m_ui.dataPlot);

    m_ui.dataPlot->replot();
  }

  void ALCDataLoadingView::displayError(const std::string& error)
  {
    QMessageBox::critical(m_widget, "Loading error", QString::fromStdString(error));
  }

  void ALCDataLoadingView::setAvailableLogs(const std::vector<std::string>& logs)
  {
    // Keep the current log value
    QString previousLog = m_ui.log->currentText();

    // Clear previous log list
    m_ui.log->clear();

    // If previousLog is in 'logs' list, add it at the beginning
    if ( std::find(logs.begin(), logs.end(), previousLog.toStdString()) != logs.end())
    {
      m_ui.log->addItem(previousLog);
    }

    // Add new items
    for (auto it = logs.begin(); it != logs.end(); ++it)
    {
      m_ui.log->addItem(QString::fromStdString(*it));
    }
  }

  void ALCDataLoadingView::setAvailablePeriods(const std::vector<std::string>& periods)
  {
    // Clear previous list
    m_ui.redPeriod->clear();
    m_ui.greenPeriod->clear();

    // Add new items
    for (auto it=periods.begin(); it!=periods.end(); ++it)
    {
      m_ui.redPeriod->addItem(QString::fromStdString(*it));
      m_ui.greenPeriod->addItem(QString::fromStdString(*it));
    }
  }

  void ALCDataLoadingView::setTimeLimits(double tMin, double tMax)
  {
    // Set initial values
    m_ui.minTime->setValue(tMin);
    m_ui.maxTime->setValue(tMax);
  }

  void ALCDataLoadingView::setTimeRange(double tMin, double tMax)
  {
    // Set range for minTime
    m_ui.minTime->setMinimum(tMin);
    m_ui.minTime->setMaximum(tMax);
    // Set range for maxTime
    m_ui.maxTime->setMinimum(tMin);
    m_ui.maxTime->setMaximum(tMax);
  }

  void ALCDataLoadingView::help()
  {
    MantidQt::API::HelpWindow::showCustomInterface(NULL, QString("Muon_ALC"));
  }

  void ALCDataLoadingView::disableAll() {

    // Disable all the widgets in the view
    m_ui.dataGroup->setEnabled(false);
    m_ui.deadTimeGroup->setEnabled(false);
    m_ui.detectorGroupingGroup->setEnabled(false);
    m_ui.periodsGroup->setEnabled(false);
    m_ui.calculationGroup->setEnabled(false);
    m_ui.load->setEnabled(false);

  }

  void ALCDataLoadingView::enableAll() {

    // Enable all the widgets in the view
    m_ui.deadTimeGroup->setEnabled(true);
    m_ui.dataGroup->setEnabled(true);
    m_ui.detectorGroupingGroup->setEnabled(true);
    m_ui.periodsGroup->setEnabled(true);
    m_ui.calculationGroup->setEnabled(true);
    m_ui.load->setEnabled(true);

  }

} // namespace CustomInterfaces
} // namespace MantidQt

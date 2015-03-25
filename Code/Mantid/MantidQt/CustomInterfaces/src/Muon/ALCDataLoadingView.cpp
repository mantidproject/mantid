#include "MantidQtCustomInterfaces/Muon/ALCDataLoadingView.h"

#include <QDesktopServices>
#include <QMessageBox>
#include <QUrl>

#include <qwt_symbol.h>

namespace MantidQt
{
namespace CustomInterfaces
{
  ALCDataLoadingView::ALCDataLoadingView(QWidget* widget)
    : m_widget(widget), m_dataCurve(new QwtPlotCurve())
  {}

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

  void ALCDataLoadingView::setDataCurve(const QwtData& data)
  {
    m_dataCurve->setData(data);
    m_ui.dataPlot->replot();
  }

  void ALCDataLoadingView::displayError(const std::string& error)
  {
    QMessageBox::critical(m_widget, "Loading error", QString::fromStdString(error));
  }

  void ALCDataLoadingView::setAvailableLogs(const std::vector<std::string>& logs)
  {
    // Clear previous log list
    m_ui.log->clear();

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

  void ALCDataLoadingView::help()
  {
    QDesktopServices::openUrl(QUrl(QString("http://www.mantidproject.org/") +
            "Muon_ALC:_Data_Loading"));
  }

  void ALCDataLoadingView::setWaitingCursor()
  {
    QApplication::setOverrideCursor(Qt::WaitCursor);
  }

  void ALCDataLoadingView::restoreCursor()
  {
    QApplication::restoreOverrideCursor();
  }

} // namespace CustomInterfaces
} // namespace MantidQt

//------------------------------------------------------
// Includes
//------------------------------------------------------
#include "MantidQtMantidWidgets/PreviewPlot.h"

#include <Poco/Notification.h>
#include <Poco/NotificationCenter.h>
#include <Poco/AutoPtr.h>
#include <Poco/NObserver.h>

#include <QBrush>
#include <QHBoxLayout>

using namespace MantidQt::MantidWidgets;


PreviewPlot::PreviewPlot(QWidget *parent, bool init) : API::MantidWidget(parent),
  m_removeObserver(*this, &PreviewPlot::handleRemoveEvent),
  m_replaceObserver(*this, &PreviewPlot::handleReplaceEvent),
  m_init(init), m_allowPan(false), m_allowZoom(false),
  m_plot(NULL), m_curves(),
  m_magnifyTool(NULL), m_panTool(NULL), m_zoomTool(NULL)
{
  if(init)
  {
    Mantid::API::AnalysisDataServiceImpl& ads = Mantid::API::AnalysisDataService::Instance();
    ads.notificationCenter.addObserver(m_removeObserver);
    ads.notificationCenter.addObserver(m_replaceObserver);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setSizeConstraint(QLayout::SetNoConstraint);

    m_plot = new QwtPlot(this);
    m_plot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mainLayout->addWidget(m_plot);

    this->setLayout(mainLayout);
  }
}


/**
 * Destructor
 *
 * Removes observers on the ADS.
 */
PreviewPlot::~PreviewPlot()
{
  if(m_init)
  {
    Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_removeObserver);
    Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_replaceObserver);
  }
}


/**
 * Gets the background colour of the plot window.
 *
 * @return Plot canvas colour
 */
QColor PreviewPlot::canvasColour()
{
  if(m_plot)
    return m_plot->canvasBackground();

  return QColor();
}


/**
 * Sets the background colour of the plot window.
 *
 * @param colour Plot canvas colour
 */
void PreviewPlot::setCanvasColour(const QColor & colour)
{
  if(m_plot)
    m_plot->setCanvasBackground(QBrush(colour));
}


/**
 * Enables or disables the option to use the pan tool on the plot.
 *
 * @param allow If tool should be allowed
 */
void PreviewPlot::setAllowPan(bool allow)
{
  m_allowPan = allow;
}


/**
 * Checks to see if the option to use the pan tool is enabled.
 *
 * @return True if tool is allowed
 */
bool PreviewPlot::allowPan()
{
  return m_allowPan;
}


/**
 * Enables or disables the option to use the zoom tool on the plot.
 *
 * @param allow If tool should be allowed
 */
void PreviewPlot::setAllowZoom(bool allow)
{
  m_allowZoom = allow;
}


/**
 * Checks to see if the option to use the zoom tool is enabled.
 *
 * @return True if tool is allowed
 */
bool PreviewPlot::allowZoom()
{
  return m_allowZoom;
}


void PreviewPlot::addSpectra(Mantid::API::MatrixWorkspace_sptr ws, int specIndex)
{
}


void PreviewPlot::addSpectra(const QString & wsName, int specIndex)
{
}


void PreviewPlot::removeSpectra(Mantid::API::MatrixWorkspace_sptr ws)
{
}


void PreviewPlot::removeSpectra(const QString & wsName)
{
}


void PreviewPlot::replot()
{
}


void PreviewPlot::handleRemoveEvent(Mantid::API::WorkspacePreDeleteNotification_ptr pNf)
{
}


void PreviewPlot::handleReplaceEvent(Mantid::API::WorkspaceAfterReplaceNotification_ptr pNf)
{
}

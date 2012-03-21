#include "ThreadAdapter.h"
#include "ApplicationWindow.h"
#include "Mantid/MantidUI.h"

#include <QCoreApplication>
#include <QThread>

/**
 * Contructor
 * @param appWindow :: A reference to the ApplicationWindow
 * @param mantidUI :: A reference to the MantidUI instance
 */
ThreadAdapter::ThreadAdapter(ApplicationWindow & appWindow, MantidUI & mantidUI)
 : QObject(), m_appWindow(appWindow), m_mantidUI(mantidUI),
   m_lastWidget(NULL)
{
  // This object must live on the GUI thread so that the slots that are called
  // will be called in that thread
  this->moveToThread(QCoreApplication::instance()->thread());
}

MultiLayer * ThreadAdapter::plotSpectraList(const QStringList& wsNames, const QList<int>& spectrumList,
                                            bool errs, Graph::CurveType style)
{
  if( QThread::currentThread() != QCoreApplication::instance()->thread() )
  {
    bool methodSuccess = QMetaObject::invokeMethod(this, "plotSpectraListGUIThread", Qt::BlockingQueuedConnection,
                                            Q_ARG(const QStringList&, wsNames), Q_ARG(const QList<int>&, spectrumList),
                                            Q_ARG(bool, errs), Q_ARG(Graph::CurveType, style));
    if(!methodSuccess)
    {
      throw std::runtime_error("Error invoking plotSpectraList from separate thread.");
    }
    MultiLayer *result = qobject_cast<MultiLayer*>(m_lastWidget);
    m_lastWidget = NULL;
    return result;
    }
  else
  {
    return m_mantidUI.plotSpectraList(wsNames, spectrumList, errs, style);
  }
}

void ThreadAdapter::plotSpectraListGUIThread(const QStringList& wsNames, const QList<int>& spectrumList,
                                             bool errs, Graph::CurveType style)
{
  m_lastWidget = static_cast<QWidget*>(m_mantidUI.plotSpectraList(wsNames, spectrumList, errs, style));
}

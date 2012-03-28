#include "ThreadAdapter.h"
#include "ApplicationWindow.h"
#include "Mantid/MantidUI.h"

#include <QCoreApplication>
#include <QThread>

#define BEGIN_THREAD_CHECK(ReturnType, DefaultValue) \
  ReturnType result(DefaultValue);\
  if( QThread::currentThread() != QCoreApplication::instance()->thread() )\
  {

#define END_THREAD_CHECK(ReturnType) \
   if(!methodSuccess)\
   {\
     throw std::runtime_error("Error invoking method from separate thread.");\
   }\
   result = qobject_cast<ReturnType>(m_lastWidget);\
   m_lastWidget = NULL;\
  }

#define DO_GUI_THREAD_CALL(GuiFunctionCall) \
  else\
  {\
    m_lastWidget = static_cast<QWidget*>(GuiFunctionCall);\
  }\
  return result;

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
  BEGIN_THREAD_CHECK(MultiLayer*, NULL)
  bool methodSuccess = QMetaObject::invokeMethod(this, "plotSpectraList", Qt::BlockingQueuedConnection,
                                                 Q_ARG(const QStringList&, wsNames), Q_ARG(const QList<int>&, spectrumList),
                                                 Q_ARG(bool, errs), Q_ARG(Graph::CurveType, style));
  END_THREAD_CHECK(MultiLayer*)
  DO_GUI_THREAD_CALL(m_mantidUI.plotSpectraList(wsNames, spectrumList, errs, style))
}

MultiLayer* ThreadAdapter::plotBin(const QString& wsName,int index,bool errs, Graph::CurveType style)
{
  BEGIN_THREAD_CHECK(MultiLayer*, NULL)
  bool methodSuccess = QMetaObject::invokeMethod(this, "plotBin", Qt::BlockingQueuedConnection,
                                                 Q_ARG(const QString&, wsName), Q_ARG(int, index),
                                                 Q_ARG(bool, errs), Q_ARG(Graph::CurveType, style));
  END_THREAD_CHECK(MultiLayer*)
  DO_GUI_THREAD_CALL(m_mantidUI.plotBin(wsName, index, errs, style))
}

MultiLayer* ThreadAdapter::mergePlots(MultiLayer *plotOne, MultiLayer *plotTwo)
{
  BEGIN_THREAD_CHECK(MultiLayer*, NULL)
  bool methodSuccess = QMetaObject::invokeMethod(this, "plotBin", Qt::BlockingQueuedConnection,
                                                 Q_ARG(MultiLayer*, plotOne), Q_ARG(MultiLayer*, plotTwo));
  END_THREAD_CHECK(MultiLayer*)
  DO_GUI_THREAD_CALL(m_mantidUI.mergePlots(plotOne, plotTwo))
}

//void convertToWaterfall(MultiLayer *simplePlot);


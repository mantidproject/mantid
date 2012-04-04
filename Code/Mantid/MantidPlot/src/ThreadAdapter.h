#ifndef THREADADAPTER_H_
#define THREADADAPTER_H_

#include <QObject>

#include "Graph.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class QWidget;
class ApplicationWindow;
class MantidUI;
class MantidMatrix;
class Table;

/**
 * An object that ensures all function calls that occur end up on the
 * main GUI thread.
 *
 * The idea is simple. If there is a function that can only be called from
 * the GUI thread then these set of functions ensure that the call always
 * happens on that thread. Essentially each function defined here is a
 * proxy for a standard function call defined on another object. That function
 * must be a declared as a slot on the source object.
 *
 * Each call should proceed as follows:
 *  1) First check whether the current thread is the GUI thread;
 *  2) If we are on the GUI thread, proceed with the standard function call
 *  3) If we are on a different thread then use QMetaObject::invokeMethod,
 *     with a Qt::BlockingQueuedConnection, to call the standard function
 *     so that it is called on the GUI thread. If a return value is required
 *     then this must be set as an instance variable and retrieved after
 *     the asynchronous function call
 */
class ThreadAdapter : public QObject
{
  Q_OBJECT
public:
  /// Construct with an ApplicationWindow & MantidUI instance
  explicit ThreadAdapter(ApplicationWindow & appWindow, MantidUI & mantidUI);

public slots:
  //-------------------------- Plotting -------------------------------------------------------
  MultiLayer * plotSpectraList(const QStringList& wsNames, const QList<int>& spectrumList,
                               bool errs = true, Graph::CurveType style = Graph::Unspecified);
  MultiLayer* plotBin(const QString& wsName,int index,bool errs = false,
                      Graph::CurveType style = Graph::Unspecified);
  MultiLayer* mergePlots(MultiLayer *plotOne, MultiLayer *plotTwo);
//  void convertToWaterfall(MultiLayer *simplePlot);

  //-------------------------- Instrument view ------------------------------------------------
  //  InstrumentWindow* getInstrumentView(const QString &, int tab = -1);
  //%MethodCode
  //  sipRes = sipCpp->getInstrumentView(*a0,a1);
  //  sipRes->hide();
  //%End

  //-------------------------- Mantid Matrices -------------------------------------------------
  // Methods relating to creating or getting handles to GUI objects
//  MantidMatrix* getMantidMatrix(const QString &);
//  MantidMatrix* importMatrixWorkspace(const QString& wsName, int lower=-1, int upper=-1,
//                                      bool showDlg=false, bool makeVisible=false);
//
//  //-------------------------- Tables ----------------------------------------------------------
//  Table* createTableFromSpectraList(const QString& tableName, const QString& workspaceName,
//                                    QList<int> indexList, bool errs=true, bool binCentres=false);
//  Table* importTableWorkspace(const QString&, bool = false, bool = false);

  //--------------------------- Dialog functions ----------------------------------------------
  bool createPropertyInputDialog(const QString & alg_name, const QString & preset_values,
                                 const QString & optional_msg, const QStringList & enabled, 
                                 const QStringList & disabled);

private:
  // Ban default construction & copy
  ThreadAdapter();
  Q_DISABLE_COPY(ThreadAdapter);

  /// Main application window instance
  ApplicationWindow & m_appWindow;
  /// MantidUI instance
  MantidUI & m_mantidUI;

  /// A pointer to the last widget object created
  QWidget *m_lastWidget;
  /// Store the return value of the last boolean return
  bool m_lastBool;
};

#endif /* ThreadAdapter_H_ */

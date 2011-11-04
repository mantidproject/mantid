#ifndef MANTIDQTCUSTOMINTERFACES_STANDARD_LOG_VIEW_H_
#define MANTIDQTCUSTOMINTERFACES_STANDARD_LOG_VIEW_H_

//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/LogView.h"
#include <boost/scoped_ptr.hpp>
#include <qwidget.h>

class QTableWidget;
namespace MantidQt
{
  namespace CustomInterfaces
  {
    class LogPresenter;

    /** Concrete LogView as a QtWidget 
    */
    class StandardLogView : public QWidget, public LogView
    {
      Q_OBJECT

    private:

      /// MVP presenter
      boost::shared_ptr<LogPresenter> m_presenter;

      /// Default/Cached palette.
      QPalette m_pal;

      /// Flag indicating that editing has been requested.
      bool m_request_edit;

      /// table widget;
      QTableWidget* m_tableWidget;

    private slots:

      void edited();

    public:

      /// Constructor
      StandardLogView(boost::shared_ptr<LogPresenter> presenter);

      /// Destructor
      ~StandardLogView();

      /// Indicate that the view has been modified.
      void indicateModified();

      /// Indicate that the view is unmodified.
      void indicateDefault();

      /// Initalization method.
      void initalize(std::vector<AbstractMementoItem_sptr>);

      /// Getter for the log data.
      virtual LogDataMap getLogData() const;

      /// Getter for the edit request status.
      virtual bool getRequestEdit() const;

    };
  }
}

#endif
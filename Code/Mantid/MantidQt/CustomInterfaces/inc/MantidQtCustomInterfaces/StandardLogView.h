#ifndef MANTIDQTCUSTOMINTERFACES_STANDARD_LOG_VIEW_H_
#define MANTIDQTCUSTOMINTERFACES_STANDARD_LOG_VIEW_H_

//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/LogView.h"
#include <boost/scoped_ptr.hpp>
#include <qwidget.h>

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
      boost::scoped_ptr<LogPresenter> m_presenter;

      /// Default/Cached palette.
      QPalette m_pal;

    private slots:

      void edited();

    public:

      /// Constructor
      StandardLogView(LogPresenter* presenter);

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

    };
  }
}

#endif
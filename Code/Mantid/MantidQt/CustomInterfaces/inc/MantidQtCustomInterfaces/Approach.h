#ifndef MANTIDQTCUSTOMINTERFACES_APPROACH_H_
#define MANTIDQTCUSTOMINTERFACES_APPROACH_H_

#include <QWidget>
#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>

namespace MantidQt
{
  namespace CustomInterfaces
  {
    class LatticePresenter;
    class LogPresenter;

    class DLLExport Approach
    {
    public:
      virtual QWidget* createLatticeView(boost::shared_ptr<LatticePresenter> presenter) = 0;
      virtual QWidget* createLogView(boost::shared_ptr<LogPresenter> presenter) = 0;
      virtual QWidget* createEditableLogView(boost::shared_ptr<LogPresenter> presenter) = 0;
    };

    /// Typdef approach wrapped in shared pointer.
    typedef boost::shared_ptr<Approach> Approach_sptr;
    /// Typdef presenter wrapped in shared pointer.
    typedef boost::shared_ptr<LatticePresenter> LatticePresenter_sptr;
    /// Typdef presenter wrapped in shared pointer.
    typedef boost::shared_ptr<LogPresenter> LogPresenter_sptr;
  }
}

#endif
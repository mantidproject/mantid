#ifndef MANTIDQTCUSTOMINTERFACES_APPROACH_H_
#define MANTIDQTCUSTOMINTERFACES_APPROACH_H_

#include <qwidget.h>
#include "MantidKernel/System.h"
#include "MantidQtCustomInterfaces/LatticeView.h"
#include "MantidQtCustomInterfaces/LoanedMemento.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    class DLLExport Approach
    {
    public:
      virtual QWidget* createLatticeView() = 0;
    };
  }
}

#endif
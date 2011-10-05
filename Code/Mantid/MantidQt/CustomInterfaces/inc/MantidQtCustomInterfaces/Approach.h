#ifndef MANTIDQTCUSTOMINTERFACES_APPROACH_H_
#define MANTIDQTCUSTOMINTERFACES_APPROACH_H_

#include <qwidget.h>
#include "MantidQtCustomInterfaces/LatticeView.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    class Approach
    {
    public:
      virtual QWidget* createLatticeView() = 0;
    };
  }
}

#endif
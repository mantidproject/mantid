#ifndef MANTIDQTWIDGETS_ICATHELPER_H_
#define MANTIDQTWIDGETS_ICATHELPER_H_

#include "MantidAPI/AlgorithmManager.h"

namespace MantidQt
{
  namespace MantidWidgets
  {
    class ICatHelper
    {

    public:
      /// Obtain the list of instruments that are available.
      std::vector<std::string> getInstrumentList();
    };
  }
}
#endif // MANTIDQTWIDGETS_ICATHELPER_H_

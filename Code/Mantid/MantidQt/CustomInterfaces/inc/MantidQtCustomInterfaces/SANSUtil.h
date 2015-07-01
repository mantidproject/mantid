#ifndef MANTIDQTCUSTOMINTERFACES_SANSUTILITY_H_
#define MANTIDQTCUSTOMINTERFACES_SANSUTILITY_H_
#include "MantidQtCustomInterfaces/DllConfig.h"
#include <QString>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_CUSTOMINTERFACES_DLL SANSUtil {
public:
  SANSUtil();
  ~SANSUtil();
  QString createPythonStringList(QString input, QString delimiter);
};
}
}

#endif
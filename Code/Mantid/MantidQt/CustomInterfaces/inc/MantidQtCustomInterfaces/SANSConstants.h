#ifndef MANTIDQTCUSTOMINTERFACES_SANSCONSTANTS_H_
#define MANTIDQTCUSTOMINTERFACES_SANSCONSTANTS_H_

#include <QString>

namespace MantidQt
{
namespace CustomInterfaces
{

class SANSConstants
{
public:
  SANSConstants();
  ~SANSConstants();
  QString getPythonSuccessKeyword();
  QString getPythonEmptyKeyword();
  QString getPythonTrueKeyword();
};

}
}

#endif  //MANTIDQTCUSTOMINTERFACES_SANSADDFILES_H_
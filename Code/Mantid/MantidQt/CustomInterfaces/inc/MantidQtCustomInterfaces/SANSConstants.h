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

  // Python related
  QString getPythonSuccessKeyword();
  QString getPythonEmptyKeyword();
  QString getPythonTrueKeyword();

  // Input related
  double getMaxDoubleValue();
  int getMaxIntValue();
  int getDecimals();
};

}
}

#endif  //MANTIDQTCUSTOMINTERFACES_SANSADDFILES_H_
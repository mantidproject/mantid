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
  QString getPythonFalseKeyword();

  QString getQResolutionH1ToolTipText();
  QString getQResolutionH2ToolTipText();
  QString getQResolutionA1ToolTipText();
  QString getQResolutionA2ToolTipText();
};

}
}

#endif  //MANTIDQTCUSTOMINTERFACES_SANSADDFILES_H_
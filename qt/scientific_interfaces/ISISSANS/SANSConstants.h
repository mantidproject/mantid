#ifndef MANTIDQTCUSTOMINTERFACES_SANSCONSTANTS_H_
#define MANTIDQTCUSTOMINTERFACES_SANSCONSTANTS_H_

#include <QString>

namespace MantidQt {
namespace CustomInterfaces {

class SANSConstants {
public:
  SANSConstants();
  ~SANSConstants();

  // Python related
  static QString getPythonSuccessKeyword();
  static QString getPythonEmptyKeyword();
  static QString getPythonTrueKeyword();
  static QString getPythonFalseKeyword();

  static QString getQResolutionH1ToolTipText();
  static QString getQResolutionH2ToolTipText();
  static QString getQResolutionA1ToolTipText();
  static QString getQResolutionA2ToolTipText();

  // Input related
  static double getMaxDoubleValue();
  static int getMaxIntValue();
  static int getDecimals();
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_SANSADDFILES_H_
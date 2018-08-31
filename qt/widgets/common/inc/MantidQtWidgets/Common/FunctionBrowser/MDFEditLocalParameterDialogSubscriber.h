#ifndef MDFEDITLOCALPARAMETERDIALOGSUBSCRIBER_H_
#define MDFEDITLOCALPARAMETERDIALOGSUBSCRIBER_H_

#include <string>

namespace MantidQt {
namespace CustomInterfaces {
namespace MDF {

class EditLocalParameterDialogSubscriber {
public:
  virtual void setParameters(double value) = 0;
  virtual void setFixed(bool fixed) = 0;
  virtual void setTies(std::string const &tie) = 0;
  virtual void setParameter(double value, int index) = 0;
  virtual void fixParameter(bool fixed, int index) = 0;
  virtual void setTie(std::string const &tie, int index) = 0;
  virtual void copyValuesToClipboard() = 0;
  virtual void pasteValuesFromClipboard(std::string const &text) = 0;
  virtual void setValuesToLog(std::string const &logName,
                              std::string const &mode) = 0;
  virtual void setValueToLog(std::string const &logName,
                             std::string const &mode, int index) = 0;
};

} // namespace MDF
} // namespace CustomInterfaces
} // namespace MantidQt

#endif

// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IMESSAGEHANDLER_H
#define MANTID_ISISREFLECTOMETRY_IMESSAGEHANDLER_H

#include <string>
namespace MantidQt {
namespace CustomInterfaces {
/** @class IMessageHandler

IMessageHandler is an interface for passing messages to the user
*/
class IMessageHandler {
public:
  virtual ~IMessageHandler(){};
  virtual void giveUserCritical(const std::string &prompt,
                                const std::string &title) = 0;
  virtual void giveUserInfo(const std::string &prompt,
                            const std::string &title) = 0;
  virtual bool askUserYesNo(const std::string &prompt,
                            const std::string &title) = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif

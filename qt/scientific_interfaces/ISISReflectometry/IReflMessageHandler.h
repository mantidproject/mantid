// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IREFLMESSAGEHANDLER_H
#define MANTID_ISISREFLECTOMETRY_IREFLMESSAGEHANDLER_H

#include <string>

namespace MantidQt {
namespace CustomInterfaces {
/** @class IReflMessageHandler

IReflMessageHandler is an interface for passing messages to the user
*/
class IReflMessageHandler {
public:
  virtual ~IReflMessageHandler(){};
  virtual void giveUserCritical(const std::string &prompt,
                                const std::string &title) = 0;
  virtual void giveUserInfo(const std::string &prompt,
                            const std::string &title) = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif

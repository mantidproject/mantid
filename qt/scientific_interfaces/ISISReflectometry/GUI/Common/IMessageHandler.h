// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <string>
namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
/** @class IMessageHandler

IMessageHandler is an interface for passing messages to the user
*/
class IMessageHandler {
public:
  virtual ~IMessageHandler(){};
  virtual void giveUserCritical(const std::string &prompt, const std::string &title) = 0;
  virtual void giveUserInfo(const std::string &prompt, const std::string &title) = 0;
  virtual bool askUserOkCancel(const std::string &prompt, const std::string &title) = 0;
  virtual std::string askUserForSaveFileName(std::string const &filter) = 0;
  virtual std::string askUserForLoadFileName(std::string const &filter) = 0;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt

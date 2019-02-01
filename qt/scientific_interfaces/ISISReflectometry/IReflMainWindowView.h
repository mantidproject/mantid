// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IREFLMAINWINDOWVIEW_H
#define MANTID_ISISREFLECTOMETRY_IREFLMAINWINDOWVIEW_H

#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/** @class IReflMainWindowView

IReflMainWindowView is the interface defining the functions that the main
window view needs to implement. It is empty and not necessary at the moment, but
can be used in the future if widgets common to all tabs are added, for instance,
the help button.
*/
class IReflMainWindowView {
public:
  /// Destructor
  virtual ~IReflMainWindowView(){};

  /// Dialog to show an error message
  virtual void giveUserCritical(const std::string &prompt,
                                const std::string &title) = 0;
  /// Dialog to show information
  virtual void giveUserInfo(const std::string &prompt,
                            const std::string &title) = 0;
  /// Run a python algorithm
  virtual std::string runPythonAlgorithm(const std::string &pythonCode) = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IREFLMAINWINDOWVIEW_H */

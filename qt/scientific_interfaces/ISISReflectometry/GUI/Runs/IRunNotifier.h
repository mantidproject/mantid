// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IREFLRUNNOTIFIER_H
#define MANTID_ISISREFLECTOMETRY_IREFLRUNNOTIFIER_H

#include <string>

namespace MantidQt {
namespace CustomInterfaces {
/** @class IRunNotifier

IRunNotifier is an interface for polling for runs from IRunsPresenter
implementations.
*/
class IRunNotifier {
public:
  virtual ~IRunNotifier(){};
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif

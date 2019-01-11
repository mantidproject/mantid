// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidVatesAPI/ProgressAction.h"

namespace Mantid {
namespace VATES {

ProgressAction::ProgressAction() {}

void ProgressAction::handler(
    const Poco::AutoPtr<Mantid::API::Algorithm::ProgressNotification> &pNf) {
  this->eventRaised(pNf->progress);
}
} // namespace VATES
} // namespace Mantid

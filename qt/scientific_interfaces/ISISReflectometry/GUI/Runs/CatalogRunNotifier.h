// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_CATALOGRUNNOTIFIER_H
#define MANTID_ISISREFLECTOMETRY_CATALOGRUNNOTIFIER_H

#include "IRunNotifier.h"

namespace MantidQt {
namespace CustomInterfaces {
/** @class CatalogRunNotifier

CatalogRunNotifier implements IRunNotifier to provide functionality to
poll for new runs.
*/
class CatalogRunNotifier : public IRunNotifier {
public:
  ~CatalogRunNotifier() override{};
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif

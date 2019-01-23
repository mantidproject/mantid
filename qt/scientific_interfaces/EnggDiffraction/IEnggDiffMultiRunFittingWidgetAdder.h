// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_IENGGDIFFMULTIRUNFITTINGWIDGETADDER_H_
#define MANTIDQT_CUSTOMINTERFACES_IENGGDIFFMULTIRUNFITTINGWIDGETADDER_H_

#include "IEnggDiffMultiRunFittingWidgetOwner.h"

namespace MantidQt {
namespace CustomInterfaces {

class IEnggDiffMultiRunFittingWidgetAdder {

public:
  virtual ~IEnggDiffMultiRunFittingWidgetAdder() = default;
  virtual void operator()(IEnggDiffMultiRunFittingWidgetOwner &owner) = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQT_CUSTOMINTERFACES_IENGGDIFFMULTIRUNFITTINGWIDGETADDER_H_

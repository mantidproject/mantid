// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

class IEnggDiffMultiRunFittingWidgetView;

/// Interface for a view which contains a multi-run fitting widget
class IEnggDiffMultiRunFittingWidgetOwner {
public:
  /// Add a multi-run widget to the owner
  virtual void addWidget(IEnggDiffMultiRunFittingWidgetView *widget) = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

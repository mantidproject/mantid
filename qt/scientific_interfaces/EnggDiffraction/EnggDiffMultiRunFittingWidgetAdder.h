// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETADDER_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETADDER_H_

#include "IEnggDiffMultiRunFittingWidgetAdder.h"

#include <boost/shared_ptr.hpp>

namespace MantidQt {
namespace CustomInterfaces {

class EnggDiffMultiRunFittingWidgetView;

/// Functor for encapsulating a multi-run fitting widget view and adding it to a
/// parent
class EnggDiffMultiRunFittingWidgetAdder
    : public IEnggDiffMultiRunFittingWidgetAdder {
public:
  explicit EnggDiffMultiRunFittingWidgetAdder(
      IEnggDiffMultiRunFittingWidgetView *widget);

  /// Add the widget to an owner
  void operator()(IEnggDiffMultiRunFittingWidgetOwner &owner) override;

private:
  IEnggDiffMultiRunFittingWidgetView *m_widget;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETADDER_H_

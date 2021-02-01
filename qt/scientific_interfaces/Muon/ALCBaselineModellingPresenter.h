// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"

#include "DllConfig.h"
#include "IALCBaselineModellingModel.h"
#include "IALCBaselineModellingView.h"

#include <QObject>

namespace MantidQt {
namespace CustomInterfaces {

/** ALCBaselineModellingPresenter : Presenter for ALC Baseline Modelling step
 */
class MANTIDQT_MUONINTERFACE_DLL ALCBaselineModellingPresenter : public QObject {
  Q_OBJECT

public:
  ALCBaselineModellingPresenter(IALCBaselineModellingView *view, IALCBaselineModellingModel *model);

  void initialize();

private slots:
  /// Perform a fit
  void fit();

  /// Add a new section
  void addSection();

  /// Remove existing section
  void removeSection(int row);

  /// Called when one of sections is modified
  void onSectionRowModified(int row);

  /// Called when on of section selectors is modified
  void onSectionSelectorModified(int index);

  /// Updates data curve from the model
  void updateDataCurve();

  /// Updates corrected data curve from the model
  void updateCorrectedCurve();

  /// Updated baseline curve from the model
  void updateBaselineCurve();

  /// Updates function in the view from the model
  void updateFunction();

private:
  /// Associated view
  IALCBaselineModellingView *const m_view;

  /// Associated model
  IALCBaselineModellingModel *const m_model;
};

} // namespace CustomInterfaces
} // namespace MantidQt

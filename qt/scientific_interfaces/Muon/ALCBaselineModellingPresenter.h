// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IALCBaselineModellingModel.h"
#include "IALCBaselineModellingPresenter.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidKernel/System.h"

#include "DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {

class IALCBaselineModellingView;

/** ALCBaselineModellingPresenter : Presenter for ALC Baseline Modelling step
 */
class MANTIDQT_MUONINTERFACE_DLL ALCBaselineModellingPresenter : IALCBaselineModellingPresenter {

public:
  ALCBaselineModellingPresenter(IALCBaselineModellingView *view, std::unique_ptr<IALCBaselineModellingModel> model);

  void initialize() override;

  /// Perform a fit
  void fit() override;

  /// Add a new section
  void addSection() override;

  /// Remove existing section
  void removeSection(int row) override;

  /// Called when one of sections is modified
  void onSectionRowModified(int row) override;

  /// Called when on of section selectors is modified
  void onSectionSelectorModified(int index) override;

  /// Updates data curve from the model
  void updateDataCurve();

  /// Updates corrected data curve from the model
  void updateCorrectedCurve();

  /// Updated baseline curve from the model
  void updateBaselineCurve();

  /// Updates function in the view from the model
  void updateFunction();

  Mantid::API::MatrixWorkspace_sptr exportWorkspace() const override;

  Mantid::API::ITableWorkspace_sptr exportSections() const override;

  Mantid::API::ITableWorkspace_sptr exportModel() const override;

  Mantid::API::MatrixWorkspace_sptr correctedData() const override;

  void setData(Mantid::API::MatrixWorkspace_sptr data) override;

  void setCorrectedData(Mantid::API::MatrixWorkspace_sptr data) override;

  std::string function() const override;

  int noOfSectionRows() const override;

private:
  void updateAfterFit();

  /// Associated view
  IALCBaselineModellingView *const m_view;

  /// Associated model
  std::unique_ptr<IALCBaselineModellingModel> const m_model;
};

} // namespace CustomInterfaces
} // namespace MantidQt

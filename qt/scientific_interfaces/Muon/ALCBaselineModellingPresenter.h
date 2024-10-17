// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IALCBaselineModellingModel.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidKernel/System.h"

#include "DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {

class IALCBaselineModellingView;

/** ALCBaselineModellingPresenter : Presenter for ALC Baseline Modelling step
 */
class MANTIDQT_MUONINTERFACE_DLL ALCBaselineModellingPresenter {

public:
  ALCBaselineModellingPresenter(IALCBaselineModellingView *view, std::unique_ptr<IALCBaselineModellingModel> model);

  void initialize();

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

  Mantid::API::MatrixWorkspace_sptr exportWorkspace();

  Mantid::API::ITableWorkspace_sptr exportSections();

  Mantid::API::ITableWorkspace_sptr exportModel();

  Mantid::API::MatrixWorkspace_sptr correctedData();

  void setData(Mantid::API::MatrixWorkspace_sptr data);

  void setCorrectedData(Mantid::API::MatrixWorkspace_sptr data);

  std::string function() const;

  int noOfSectionRows() const;

private:
  /// Associated view
  IALCBaselineModellingView *const m_view;

  /// Associated model
  std::unique_ptr<IALCBaselineModellingModel> const m_model;
};

} // namespace CustomInterfaces
} // namespace MantidQt

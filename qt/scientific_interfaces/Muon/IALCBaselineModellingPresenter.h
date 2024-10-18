// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <string>

#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/System.h"

#include "DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {

/** IALCBaselineModellingPresenter : Presenter interface for ALC BaselineModelling step
 */
class MANTIDQT_MUONINTERFACE_DLL IALCBaselineModellingPresenter {

public:
  virtual void initialize() = 0;

  /// Perform a fit
  virtual void fit() = 0;

  /// Add a new section
  virtual void addSection() = 0;

  /// Remove existing section
  virtual void removeSection(int row) = 0;

  /// Called when one of sections is modified
  virtual void onSectionRowModified(int row) = 0;

  /// Called when on of section selectors is modified
  virtual void onSectionSelectorModified(int index) = 0;

  virtual Mantid::API::MatrixWorkspace_sptr exportWorkspace() const = 0;

  virtual Mantid::API::ITableWorkspace_sptr exportSections() const = 0;

  virtual Mantid::API::ITableWorkspace_sptr exportModel() const = 0;

  virtual Mantid::API::MatrixWorkspace_sptr correctedData() const = 0;

  virtual void setData(Mantid::API::MatrixWorkspace_sptr data) = 0;

  virtual void setCorrectedData(Mantid::API::MatrixWorkspace_sptr data) = 0;

  virtual std::string function() const = 0;

  virtual int noOfSectionRows() const = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt
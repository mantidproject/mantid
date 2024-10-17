// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"

#include "DllConfig.h"

#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace MantidQt {
namespace CustomInterfaces {

/** IALCBaselineModellingModel : Model interface for ALC BaselineModelling step
 */
class MANTIDQT_MUONINTERFACE_DLL IALCBaselineModellingModel {

public:
  using Section = std::pair<double, double>;

  /**
   * @return Function produced by the last fit
   */
  virtual Mantid::API::IFunction_const_sptr fittedFunction() const = 0;

  /**
   * @return Corrected data produced by the last fit
   */
  virtual Mantid::API::MatrixWorkspace_sptr correctedData() const = 0;

  /**
   * @param function The fitting function
   * @param xValues The x values to evaluate over
   * @return The baseline model data
   */
  virtual Mantid::API::MatrixWorkspace_sptr baselineData(Mantid::API::IFunction_const_sptr function,
                                                         const std::vector<double> &xValues) const = 0;

  /**
   * @return Current data used for fitting
   */
  virtual Mantid::API::MatrixWorkspace_sptr data() const = 0;

  /**
   * Perform a fit using current data and specified function and sections.
   * Modified values returned
   * by fittedFunction and correctedData.
   * @param function :: Function to fit
   * @param sections :: Data sections to include in the fit
   */
  virtual void fit(Mantid::API::IFunction_const_sptr function, const std::vector<Section> &sections) = 0;

  /// Export data + baseline + corrected data as a single workspace
  virtual Mantid::API::MatrixWorkspace_sptr exportWorkspace() = 0;

  /// Set the data we should fit baseline for
  virtual void setData(Mantid::API::MatrixWorkspace_sptr data) = 0;

  /// Set the corrected data resulting from fit
  virtual void setCorrectedData(Mantid::API::MatrixWorkspace_sptr data) = 0;

  /// Export sections used for the last fit as a table workspace
  virtual Mantid::API::ITableWorkspace_sptr exportSections() = 0;

  /// Exports baseline model as a table workspace
  virtual Mantid::API::ITableWorkspace_sptr exportModel() = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

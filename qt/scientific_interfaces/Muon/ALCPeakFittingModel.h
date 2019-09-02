// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_ALCPEAKFITTINGMODEL_H_
#define MANTID_CUSTOMINTERFACES_ALCPEAKFITTINGMODEL_H_

#include "MantidKernel/System.h"

#include "DllConfig.h"
#include "IALCPeakFittingModel.h"
#include "MantidAPI/ITableWorkspace_fwd.h"

namespace MantidQt {
namespace CustomInterfaces {

/** ALCPeakFittingModel : Concrete model for ALC peak fitting
 */
class MANTIDQT_MUONINTERFACE_DLL ALCPeakFittingModel
    : public IALCPeakFittingModel {
public:
  // -- IALCPeakFittingModel interface
  // -----------------------------------------------------------
  Mantid::API::IFunction_const_sptr fittedPeaks() const override {
    return m_fittedPeaks;
  }
  Mantid::API::MatrixWorkspace_sptr data() const override { return m_data; }
  Mantid::API::ITableWorkspace_sptr parameterTable() const {
    return m_parameterTable;
  }

  void fitPeaks(Mantid::API::IFunction_const_sptr peaks) override;

  Mantid::API::MatrixWorkspace_sptr
  guessData(Mantid::API::IFunction_const_sptr function,
            const std::vector<double> &xValues) override;
  // -- End of IALCPeakFittingModel interface
  // ----------------------------------------------------

  /// Update the data
  void setData(Mantid::API::MatrixWorkspace_sptr newData);

  /// Export data and fitted peaks as a single workspace
  Mantid::API::MatrixWorkspace_sptr exportWorkspace();

  /// Export fitted peaks as a table workspace
  Mantid::API::ITableWorkspace_sptr exportFittedPeaks();

private:
  /// The data we are fitting peaks to
  Mantid::API::MatrixWorkspace_sptr m_data;

  /// Parameter table containing fit results
  Mantid::API::ITableWorkspace_sptr m_parameterTable;

  /// Setter for convenience
  void setFittedPeaks(Mantid::API::IFunction_const_sptr fittedPeaks);

  /// Last fitted peaks
  Mantid::API::IFunction_const_sptr m_fittedPeaks;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_CUSTOMINTERFACES_ALCPEAKFITTINGMODEL_H_ */

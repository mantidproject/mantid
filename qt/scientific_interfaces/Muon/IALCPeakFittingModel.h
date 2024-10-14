// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidKernel/System.h"

#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

namespace MantidQt {
namespace CustomInterfaces {

class IALCPeakFittingModelSubscriber;

/** IALCPeakFittingModel : ALC peak fitting step model interface.
 */
class MANTIDQT_MUONINTERFACE_DLL IALCPeakFittingModel {

public:
  /**
   * @return Last fitted peaks
   */
  virtual Mantid::API::IFunction_const_sptr fittedPeaks() const = 0;

  /**
   * @return Data we are fitting peaks to
   */
  virtual Mantid::API::MatrixWorkspace_sptr data() const = 0;

  /**
   * Fit specified peaks to the data of the model
   * @param peaks :: Function representing peaks to fit
   */
  virtual void fitPeaks(Mantid::API::IFunction_const_sptr peaks) = 0;

  /**
   * Retrieves a guess fit
   * @param function :: Function representing peaks to fit
   * @param xValues :: The x values for a guess fit
   */
  virtual Mantid::API::MatrixWorkspace_sptr guessData(Mantid::API::IFunction_const_sptr function,
                                                      const std::vector<double> &xValues) = 0;

  virtual void subscribe(std::weak_ptr<IALCPeakFittingModelSubscriber> subscriber) = 0;

signals:

  /// Signal to inform that the fitting was done and fitted peaks were updated
  void fittedPeaksChanged();

  /// Signal to inform that data was set
  void dataChanged();

  /// Signal to inform presenter of an error with fitting
  void errorInModel(const std::string &message);
};

} // namespace CustomInterfaces
} // namespace MantidQt

// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_IALCBASELINEMODELLINGMODEL_H_
#define MANTID_CUSTOMINTERFACES_IALCBASELINEMODELLINGMODEL_H_

#include "MantidKernel/System.h"

#include "DllConfig.h"

#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <QObject>

namespace MantidQt {
namespace CustomInterfaces {

/** IALCBaselineModellingModel : Model interface for ALC BaselineModelling step
 */
class MANTIDQT_MUONINTERFACE_DLL IALCBaselineModellingModel : public QObject {
  Q_OBJECT

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
  virtual Mantid::API::MatrixWorkspace_sptr
  baselineData(Mantid::API::IFunction_const_sptr function,
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
  virtual void fit(Mantid::API::IFunction_const_sptr function,
                   const std::vector<Section> &sections) = 0;

signals:

  // Signals emitted when various properties get changed
  void dataChanged();
  void fittedFunctionChanged();
  void correctedDataChanged();
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_CUSTOMINTERFACES_IALCBASELINEMODELLINGMODEL_H_ */

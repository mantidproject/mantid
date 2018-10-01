#ifndef MANTID_ISISREFLECTOMETRY_EXPERIMENTPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_EXPERIMENTPRESENTER_H

#include "../../DllConfig.h"
#include "../../Reduction/Experiment.h"
#include "../../ValidationResult.h"
#include "IExperimentPresenter.h"
#include "IExperimentView.h"
#include "IReflBatchPresenter.h"
#include "PerThetaDefaultsTableValidationError.h"
#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {

class ExperimentValidationErrors {
public:
  explicit ExperimentValidationErrors(
      // cppcheck-suppress passedByValue
      PerThetaDefaultsTableValidationError perThetaDefaultsErrors)
      : m_perThetaDefaultsErrors(std::move(perThetaDefaultsErrors)) {}

  PerThetaDefaultsTableValidationError const &perThetaValidationErrors() const {
    return m_perThetaDefaultsErrors;
  }

private:
  PerThetaDefaultsTableValidationError m_perThetaDefaultsErrors;
};

using ExperimentValidationResult =
    ValidationResult<Experiment, ExperimentValidationErrors>;

/** @class ExperimentPresenter

ExperimentPresenter is a presenter class for the widget 'Experiment' in the
ISIS Reflectometry Interface.

Copyright &copy; 2011-16 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL ExperimentPresenter
    : public ExperimentViewSubscriber,
      public IExperimentPresenter {
public:
  // TODO Inject the Experiment model into the constructor.
  ExperimentPresenter(IExperimentView *view, Experiment experiment,
                      double defaultsThetaTolerance);

  Experiment const &experiment() const;

  void notifySettingsChanged() override;
  void notifySummationTypeChanged() override;
  void notifyNewPerAngleDefaultsRequested() override;
  void notifyRemovePerAngleDefaultsRequested(int index) override;
  void notifyPerAngleDefaultsChanged(int row, int column) override;

  void onReductionPaused() override;
  void onReductionResumed() override;

private:
  ExperimentValidationResult validateExperimentFromView();
  PolarizationCorrections polarizationCorrectionsFromView();
  RangeInLambda transmissionRunRangeFromView();
  std::map<std::string, std::string> stitchParametersFromView();

  ExperimentValidationResult updateModelFromView();

  void showValidationResult(ExperimentValidationResult const &result);
  void
  showPerThetaTableErrors(PerThetaDefaultsTableValidationError const &errors);

  IExperimentView *m_view;
  Experiment m_model;
  double m_thetaTolerance;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_EXPERIMENTPRESENTER_H

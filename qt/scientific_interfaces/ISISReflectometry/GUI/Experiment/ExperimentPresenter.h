#ifndef MANTID_ISISREFLECTOMETRY_EXPERIMENTPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_EXPERIMENTPRESENTER_H

#include "../../DllConfig.h"
#include "IReflBatchPresenter.h"
#include "IExperimentView.h"
#include "IExperimentPresenter.h"
#include "PerThetaDefaultsValidationResult.h"
#include "../../Reduction/Experiment.h"
#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {

class ExperimentValidationResult {
public:
  ExperimentValidationResult(
      PerThetaDefaultsValidationResult perThetaDefaultsResult,
      bool stitchParametersResult)
      : m_perThetaDefaultsResult(perThetaDefaultsResult),
        m_stitchParametersResult(stitchParametersResult) {}

  PerThetaDefaultsValidationResult const &perThetaDefaults() const {
    return m_perThetaDefaultsResult;
  }

  bool stitchParametersAreValid() const { return m_stitchParametersResult; }

private:
  PerThetaDefaultsValidationResult m_perThetaDefaultsResult;
  bool m_stitchParametersResult;
};

/** @class ExperimentPresenter

ExperimentPresenter is a presenter class for the widget 'Event' in the
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
  ExperimentPresenter(IExperimentView *view, double defaultsThetaTolerance);
  void notifySettingsChanged() override;
  void notifySummationTypeChanged() override;
  void notifyNewPerAngleDefaultsRequested() override;
  void notifyRemovePerAngleDefaultsRequested(int index) override;
  void notifyPerAngleDefaultsChanged(int row, int column) override;

  Experiment const &experiment() const;

private:
  PolarizationCorrections polarizationCorrectionsFromView();
  RangeInLambda transmissionRunRangeFromView();
  ExperimentValidationResult updateModelFromView();

  void showValidationResult(ExperimentValidationResult const &result);
  void showPerThetaDefaultsValidationResult(
      PerThetaDefaultsValidationResult const &result);

  IExperimentView *m_view;
  boost::optional<Experiment> m_model;
  double m_thetaTolerance;
};
}
}
#endif // MANTID_ISISREFLECTOMETRY_EXPERIMENTPRESENTER_H

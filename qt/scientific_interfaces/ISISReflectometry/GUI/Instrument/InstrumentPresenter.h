#ifndef MANTID_ISISREFLECTOMETRY_INSTRUMENTPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_INSTRUMENTPRESENTER_H

#include "../../DllConfig.h"
#include "../../Reduction/Instrument.h"
#include "IInstrumentPresenter.h"
#include "IInstrumentView.h"
#include "IReflBatchPresenter.h"
#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {

/** @class InstrumentPresenter

InstrumentPresenter is a presenter class for the widget 'Instrument' in the
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
class MANTIDQT_ISISREFLECTOMETRY_DLL InstrumentPresenter
    : public InstrumentViewSubscriber,
      public IInstrumentPresenter {
public:
  // TODO Inject the Instrument model into the constructor.
  InstrumentPresenter(IInstrumentView *view);

  Instrument const &instrument() const;

  void notifySettingsChanged() override;

  void onReductionPaused() override;
  void onReductionResumed() override;

private:
  IInstrumentView *m_view;
  boost::optional<Instrument> m_model;

  RangeInLambda wavelengthRangeFromView();
  RangeInLambda monitorBackgroundRangeFromView();
  RangeInLambda monitorIntegralRangeFromView();
  MonitorCorrections monitorCorrectionsFromView();
  DetectorCorrectionType detectorCorrectionTypeFromView();
  DetectorCorrections detectorCorrectionsFromView();
  void updateModelFromView();
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_INSTRUMENTPRESENTER_H

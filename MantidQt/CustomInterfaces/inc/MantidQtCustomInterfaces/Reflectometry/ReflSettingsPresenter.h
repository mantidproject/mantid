#ifndef MANTID_CUSTOMINTERFACES_REFLSETTINGSPRESENTER_H
#define MANTID_CUSTOMINTERFACES_REFLSETTINGSPRESENTER_H

#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidGeometry/Instrument_fwd.h"
#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflSettingsPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

// Forward decs
class IReflSettingsView;

/** @class ReflSettingsPresenter

ReflSettingsPresenter is a presenter class for the widget 'Settings' in the
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
class MANTIDQT_CUSTOMINTERFACES_DLL ReflSettingsPresenter
    : public IReflSettingsPresenter {
public:
  /// Constructor
  ReflSettingsPresenter(IReflSettingsView *view);
  /// Destructor
  ~ReflSettingsPresenter() override;
  void notify(IReflSettingsPresenter::Flag flag) override;
  void setInstrumentName(const std::string &instName) override;

  /// Returns values passed for 'Transmission run(s)'
  std::string getTransmissionRuns(bool loadRuns) const override;
  /// Returns global options for 'CreateTransmissionWorkspaceAuto'
  std::string getTransmissionOptions() const override;
  /// Returns global options for 'ReflectometryReductionOneAuto'
  std::string getReductionOptions() const override;
  /// Returns global options for 'Stitch1DMany'
  std::string getStitchOptions() const override;

private:
  void createStitchHints();
  void getExpDefaults();
  void getInstDefaults();
  void wrapWithQuotes(std::string &str) const;
  Mantid::API::IAlgorithm_sptr createReductionAlg();
  Mantid::Geometry::Instrument_const_sptr
  createEmptyInstrument(const std::string &instName);

  /// The view we are managing
  IReflSettingsView *m_view;
  /// Name of the current instrument in use
  std::string m_currentInstrumentName;
};
}
}
#endif /* MANTID_CUSTOMINTERFACES_REFLSETTINGSPRESENTER_H */

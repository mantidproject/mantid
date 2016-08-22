#ifndef MANTID_API_LIVELISTENER_H_
#define MANTID_API_LIVELISTENER_H_

#include "MantidAPI/ILiveListener.h"

namespace Mantid {
namespace API {

/**
  Base implementation for common behaviour of all live listener classes. It
  implements the ILiveListener interface.

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport LiveListener : public API::ILiveListener {
public:
  bool dataReset() override;
  void setSpectra(const std::vector<specnum_t> &specList) override;

protected:
  /// Indicates receipt of a reset signal from the DAS.
  bool m_dataReset = false;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_LIVELISTENER_H_ */

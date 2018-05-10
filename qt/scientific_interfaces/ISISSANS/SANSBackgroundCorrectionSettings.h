#ifndef MANTIDQT_CUSTOMINTERFACES_SANSBACKGROUNDCORRECTIONSETTINGS_H_
#define MANTIDQT_CUSTOMINTERFACES_SANSBACKGROUNDCORRECTIONSETTINGS_H_

#include "DllConfig.h"
#include "MantidKernel/System.h"

#include <QString>
#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {
/** SANSBackgroundCorrectionSettings: A simple communication class between for
the SANSBackgroundCorrectionWidget

Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTIDQT_ISISSANS_DLL SANSBackgroundCorrectionSettings {
public:
  SANSBackgroundCorrectionSettings(QString runNumber, bool useMean, bool useMon,
                                   QString monNumber);

  bool hasValidSettings();

  QString getRunNumber() const;
  QString getMonNumber() const;
  bool getUseMean() const;
  bool getUseMon() const;

private:
  boost::optional<bool> m_hasValidSettings;
  const QString m_runNumber;
  const bool m_useMean;
  const bool m_useMon;
  const QString m_monNumber;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /*  MANTIDQT_CUSTOMINTERFACES_SANSBACKGROUNDCORRECTIONSETTINGS_H_ */

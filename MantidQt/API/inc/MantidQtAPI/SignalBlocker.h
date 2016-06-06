#ifndef MANTID_API_SIGNALBLOCKER_H_
#define MANTID_API_SIGNALBLOCKER_H_

#include "DllOption.h"
#include <QObject>

namespace MantidQt {
namespace API {

/** SignalBlocker : RAII signal blocker. Not available in Qt until 5.3

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
template <typename Type> class EXPORT_OPT_MANTIDQT_API SignalBlocker {

private:
  /// Object to manage blocking
  Type *m_obj;

public:
  /// Constructor
  SignalBlocker(Type *obj);
  /// Destructor
  ~SignalBlocker();
  /// Overriden function like behavior.
  Type *operator->();
  /// Release management
  void release();
};

} // namespace API
} // namespace MantidQt

#endif /* MANTID_API_SIGNALBLOCKER_H_ */

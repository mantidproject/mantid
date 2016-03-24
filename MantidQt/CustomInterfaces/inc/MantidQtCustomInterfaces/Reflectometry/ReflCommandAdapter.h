#ifndef MANTID_CUSTOMINTERFACES_REFLCOMMANDADAPTER_H
#define MANTID_CUSTOMINTERFACES_REFLCOMMANDADAPTER_H

#include "MantidQtCustomInterfaces/Reflectometry/ReflCommandBase.h"
#include <QObject>

namespace MantidQt {
namespace CustomInterfaces {
/** @class ReflCommandAdapter

ReflCommandAdapter is an interface which defines the functions any data
processor interface presenter needs to support.

Copyright &copy; 2011-14 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class ReflCommandAdapter : public QObject {
  Q_OBJECT
public:
  ReflCommandAdapter(ReflCommandBase_sptr adaptee) : m_adaptee(adaptee){};
public slots:
  void call() { m_adaptee->execute(); }

private:
	ReflCommandBase_sptr m_adaptee;
};
}
}
#endif /*MANTID_CUSTOMINTERFACES_REFLCOMMANDADAPTER_H*/
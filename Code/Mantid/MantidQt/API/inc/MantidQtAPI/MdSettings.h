#ifndef MDSETTINGS_H_
#define MDSETTINGS_H_

#include "DllOption.h"
#include "MantidQtAPI/MdConstants.h"
#include <QString>

namespace MantidQt
{
  namespace API
  {
    /**
      *
      This class is for reading and persisting MD properties.

      @date 19/12/2014

      Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    class EXPORT_OPT_MANTIDQT_API MdSettings
    {
      public:

        MdSettings();

        ~MdSettings();

        /**
         * Sets if the color scale was log in the last session
         * @param logScale The state of the log scale.
         */
        void setLastSessionLogScale(bool logScale);

        /**
         * Retrieves the state of the last session's log scale.
         * @returns Was a log scale state?
         */
        bool getLastSessionLogScale();

      private:
        MdConstants m_mdConstants;

        QString m_vsiGroup;
        QString m_lblLastSessionLogScale;
    };
  }
}

#endif 

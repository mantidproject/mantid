#ifndef MANTIDQTCUSTOMINTERFACES_LOG_VIEW_H
#define MANTIDQTCUSTOMINTERFACES_LOG_VIEW_H

#include <map>
#include <vector>
#include "MantidQtCustomInterfaces/AbstractMementoItem.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {

    typedef std::map<std::string, std::string> LogDataMap;
    /** Abstract log view.
  
      @author Owen Arnold, RAL ISIS
      @date 14/Oct/2011

      Copyright &copy; 2010-11 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

      File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
      Code Documentation is available at: <http://doxygen.mantidproject.org>
     */
    class LogView 
    {
    public:
      virtual void initalize(std::vector<AbstractMementoItem_sptr>) = 0;
      virtual LogDataMap getLogData() const = 0;
      virtual void indicateModified() = 0;
      virtual void indicateDefault() = 0;
    };
  }
}

#endif
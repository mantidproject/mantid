#ifndef MANTID_CUSTOMINTERFACES_REFLTRANSFERSTRATEGY_H
#define MANTID_CUSTOMINTERFACES_REFLTRANSFERSTRATEGY_H

#include <map>
#include <string>
#include <vector>

namespace MantidQt
{
  namespace CustomInterfaces
  {

    /** ReflTransferStrategy : Provides an stratgegy for transferring runs from search results to a format suitable for processing.

    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class ReflTransferStrategy
    {
    public:
      virtual ~ReflTransferStrategy() {};

      /**
       * @param runRows : A map where the keys are the runs and the values the descriptions
       * @returns A vector of maps where each map represents a row, with values for "runs", "theta", and "group"
       */
      virtual std::vector<std::map<std::string,std::string> > transferRuns(const std::map<std::string,std::string>& runRows) = 0;
    };
  }
}

#endif

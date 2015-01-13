#ifndef MANTID_MANTIDWIDGETS_HINTSTRATEGY_H
#define MANTID_MANTIDWIDGETS_HINTSTRATEGY_H

#include <map>
#include <string>

namespace MantidQt
{
  namespace MantidWidgets
  {
    /** HintStrategy : Provides an interface for generating hints to be used by a HintingLineEdit.

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
    class HintStrategy
    {
    public:
      HintStrategy() {};
      virtual ~HintStrategy() {};

      /** Create a list of hints for auto completion

          @returns A map of keywords to short descriptions for the keyword.
       */
      virtual std::map<std::string,std::string> createHints() = 0;
    };
  }
}

#endif /* MANTID_MANTIDWIDGETS_HINTSTRATEGY_H */

#ifndef BACKGROUNDSTRATEGY_H_
#define BACKGROUNDSTRATEGY_H_

namespace Mantid
{
  namespace API
  {
    class IMDIterator;
  }
  namespace Crystal
  {
    /** BackgroundStrategy : Abstract class used for identifying elements of a IMDWorkspace that are not considered background.

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
    class BackgroundStrategy
    {
    public:
      virtual bool isBackground(Mantid::API::IMDIterator* const iterator) const = 0;
      virtual void configureIterator(Mantid::API::IMDIterator* const iterator) const = 0;
      virtual BackgroundStrategy* clone() const = 0;
      virtual ~BackgroundStrategy()
      {
      }
    };
  }
}
#endif /* BACKGROUNDSTRATEGY_H_ */

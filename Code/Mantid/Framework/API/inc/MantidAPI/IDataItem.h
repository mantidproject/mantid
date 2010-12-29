#ifndef MANTIDAPI_IDATAITEM_H
#define MANTIDAPI_IDATAITEM_H

#include "MantidKernel/System.h"

namespace Mantid
{
namespace API
{
/**
  Interface IDataItem of a Y and error value.

  \author N. Draper
    
  Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  
  File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
*/
class DLLExport IDataItem
{
  public:
  virtual const double& Y() const =0;                   ///< Returns the Y value
  virtual const double& E() const =0;                   ///< Returns the E value
  
  virtual double& Y() =0;                               ///< Returns the Y value
  virtual double& E() =0;                               ///< Returns the E value

  ///virtual destructor
  virtual ~IDataItem()
  {}
};

}  // NAMESPACE API

}  // NAMESPACE Mantid

#endif //MANTIDAPI_IDATAITEM_H

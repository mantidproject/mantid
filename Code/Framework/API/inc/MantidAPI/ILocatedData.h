#ifndef MANTIDAPI_ILOCATEDDATA_H
#define MANTIDAPI_ILOCATEDDATA_H

#include "MantidKernel/System.h"
#include "MantidAPI/IDataItem.h"

namespace Mantid
{
namespace API
{
/**
  Interface ILocatedData describes a single data item of a point data dataset.

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
  class DLLExport ILocatedData : public IDataItem
{
  public:

  virtual const double& X() const =0; ///< Returns the X value
  virtual double& X() =0;             ///< Returns the X value

  virtual const double& X2() const =0; ///< Returns the X value of the end of the histogram bin
  virtual double& X2() =0;             ///< Returns the X value of the end of the histogram bin

  virtual bool isHistogram() const =0; ///<Returns true if the data point is hastogram data and therefore has an X2.

  virtual ~ILocatedData()
  {}
};

}  // NAMESPACE API

}  // NAMESPACE Mantid

#endif //MANTIDAPI_ILOCATEDDATA_H

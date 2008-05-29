#ifndef MANTIDAPI_IDATAITEM_H
#define MANTIDAPI_IDATAITEM_H

#include "MantidKernel/System.h"

namespace Mantid
{

namespace API
{

  //forward declaration
  class IErrorHelper;

/**
  Interface IDataItem of an X value, two error values E and E2 together with a pointer to an ErrorHelper and a specta number.

  \author N. Draper
    
  Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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
  virtual const double& E2() const =0;                  ///< Returns the E2 value
  virtual const IErrorHelper* ErrorHelper() const =0;   ///< Returns a pointer to the errorhelper
  
  virtual double& Y() =0;                               ///< Returns the Y value
  virtual double& E() =0;                               ///< Returns the E value
  virtual double& E2() =0;                              ///< Returns the E2 value

  /// Returns true if the E2 values has been set
  virtual const bool isE2() const {return true;}

  ///virtual destructor
  virtual ~IDataItem()
  {}
};

}  // NAMESPACE API

}  // NAMESPACE Mantid

#endif //MANTIDAPI_IDATAITEM_H

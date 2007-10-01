#ifndef DATAHANDLINGCOMMAND_H_
#define DATAHANDLINGCOMMAND_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "../../Kernel/inc/Algorithm.h"

namespace Mantid
{
/** @class DataHandlingCommand DataHandlingCommand.h DataHandling/DataHandlingCommand.h

    DataHandlingCommand is the base class for all data handling operations/algorithms.
    It inherits from Algorithm.

    @author Russell Taylor, Tessella Support Services plc
    @date 26/09/2007
    
    Copyright © 2007 ???RAL???

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
  class DataHandlingCommand : virtual public Algorithm
  {
  public:
    
    // RJT: Empty class so far
    
    virtual ~DataHandlingCommand() {}
  };

}

#endif /*DATAHANDLINGCOMMAND_H_*/

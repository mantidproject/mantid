#ifndef SIMPLEINTEGRATION_H_
#define SIMPLEINTEGRATION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "../../Kernel/inc/Algorithm.h"

namespace Mantid
{
/** @class SimpleIntegration SimpleIntegration.h Algorithms/SimpleIntegration.h

    Takes a 2D workspace as input and sums each Histogram1D contained within
    it, storing the result as a Workspace1D

    @author Russell Taylor, Tessella Support Services plc
    @date 05/10/2007
    
    Copyright � 2007 ???RAL???

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
class SimpleIntegration : public Algorithm
{
public:
	SimpleIntegration();
	virtual ~SimpleIntegration();
	
private:
  StatusCode init();
  StatusCode exec();
  StatusCode final();
};

}

#endif /*SIMPLEINTEGRATION_H_*/

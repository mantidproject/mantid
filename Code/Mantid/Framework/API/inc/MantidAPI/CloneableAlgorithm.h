#ifndef MANTID_API_CLONEABLE_H_
#define MANTID_API_CLONEABLE_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace API
  {

    /** 
	CloneableAlgorithm implements an interface that allows inherited objects to
	be copied using the <code>clone()</code> method.

	@author Martyn Gigg, Tessella Support Services plc
	@date 12/01/2008

	Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
	Code Documentation is available at: <http://doxygen.mantidproject.org>    
    */
    class DLLExport CloneableAlgorithm : public Algorithm
    {
    public:
      ///Default constructor
      CloneableAlgorithm();
      /// Destructor
      virtual ~CloneableAlgorithm();

      /// Create a clone of this algorithm
      virtual CloneableAlgorithm* clone() = 0;

      /// Manage a call to delete for these types of object
      virtual void kill()
      {
      }

    private:
      /// Assignment operator
      CloneableAlgorithm& operator=(const CloneableAlgorithm& rhs);
      /// Copy constructor
      CloneableAlgorithm(const CloneableAlgorithm& other);
    };

  }
}

#endif //MANTID_API_CLONEABLE_H_

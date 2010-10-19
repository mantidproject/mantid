#ifndef MANTID_ALGORITHMS_VECTORMATHEMATICS
#define MANTID_ALGORITHMS_VECTORMATHEMATICS

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include <cmath>

namespace Mantid
{
	namespace MDAlgorithms
	{
		/** A base class for absorption correction algorithms.


		Grouping of static methods used to perform vector mathematics required for MDAlgorithm support. If this collection proves 
		useful elsewhere, it should be moved down the package structure.

		@author Owen Arnold, Tessella plc
		@date 19/10/2010

		Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

		template<typename T>
		void static DotProduct(T a1, T a2, T a3, T b1, T b2, T b3, std::vector<T>& result)
		{
			result.push_back(a1 * b1);
			result.push_back(a2 * b2);
			result.push_back(a3 * b3);
		}

		template<typename T>
		T static Absolute(T a1, T a2, T a3)
		{
			return sqrt((a1 * a1) + (a2 * a2) + (a3 * a3));
		}

	}
}


#endif
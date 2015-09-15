#ifndef VATES_CONFIGURATION_H
#define VATES_CONFIGURATION_H


#include "MantidKernel/System.h"
#include <string>

namespace Mantid
{
  namespace VATES
  {
	 /** Metadata container and handler to handle json data which is passed between filters and sources through 
	     VTK field data

	 @date 1/12/2014

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

	 File change history is stored at: <https://github.com/mantidproject/mantid>
	 Code Documentation is available at: <http://doxygen.mantidproject.org>
	 */
	  
	class DLLExport VatesConfigurations
	{
		public:
      VatesConfigurations();

      ~VatesConfigurations();

      int getMaxRecursionDepth();

      std::string getMetadataIdJson();

    private:
      // The maximum recursion depth when going through the box tree.
      const int maxRecursionDepth;

      // Meta data field flag 
      const std::string metaDataId;
	};
  }
}
#endif
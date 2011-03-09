

#ifndef MANTID_VATES_VTKDATASETFACTORY_H_
#define MANTID_VATES_VTKDATASETFACTORY_H_

#include "MantidKernel/System.h"
#include "boost/shared_ptr.hpp"
#include "vtkDataSet.h"

namespace Mantid
{
namespace VATES
{

/** Abstract type to generate a vtk dataset on demand from a MDWorkspace.

 @author Owen Arnold, Tessella plc
 @date 24/01/2010

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



class DLLExport vtkDataSetFactory
{

public:

  /// Constructor
  vtkDataSetFactory();

  /// Destructor
  virtual ~vtkDataSetFactory()=0;

  /// Factory Method.
  virtual vtkDataSet* create() const=0;


};

typedef boost::shared_ptr<vtkDataSetFactory> vtkDataSetFactory_sptr;

}
}


#endif

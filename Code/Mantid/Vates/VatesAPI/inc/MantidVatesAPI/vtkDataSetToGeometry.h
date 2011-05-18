#ifndef VTKDATASET_TO_GEOMETRY_H_
#define VTKDATASET_TO_GEOMETRY_H_ 

#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/System.h"
#include "MantidVatesAPI/GeometryXMLParser.h"

class vtkDataSet;
namespace Mantid
{
  namespace VATES
  {
 
 /** @class vtkDataSetToGeometry 

 Handles the extraction of dimensions from a vtkDataSet by getting at the field data and then processing the xml contained within to determine how mappings have been formed. 

 @author Owen Arnold, Tessella Support Services plc
 @date 13/05/2011

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

 File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
    class DLLExport vtkDataSetToGeometry : public GeometryXMLParser
    {

    private:

      vtkDataSet* m_dataSet;

    public:

      explicit vtkDataSetToGeometry(vtkDataSet* dataSet);

      ~vtkDataSetToGeometry();

      virtual void execute();

      vtkDataSetToGeometry(const vtkDataSetToGeometry& other);

      vtkDataSetToGeometry& operator=(const vtkDataSetToGeometry& other);

    };
  }
}

#endif
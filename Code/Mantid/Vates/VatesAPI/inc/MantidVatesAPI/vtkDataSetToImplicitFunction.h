#ifndef VTKDATASET_TO_IMPLICIT_FUNCTION_H_
#define VTKDATASET_TO_IMPLICIT_FUNCTION_H_

#include "MantidKernel/System.h"

class vtkDataSet;
namespace Mantid
{
  namespace Geometry
  {
    //Forward declaration.
    class MDImplicitFunction;
  }
  namespace VATES
  {

    /** @class vtkDataSetToImplicitFunction 

    Handles the extraction of existing implcitfunctions from a vtkDataSet by getting at the field data and then processing the xml contained within.

    @author Owen Arnold, Tessella Support Services plc
    @date 22/08/2011

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport vtkDataSetToImplicitFunction
    {
    public:
        static Mantid::Geometry::MDImplicitFunction* exec(vtkDataSet* dataSet);
        vtkDataSetToImplicitFunction(vtkDataSet* dataSet);
        Mantid::Geometry::MDImplicitFunction* execute();
        ~vtkDataSetToImplicitFunction();
    private:
        vtkDataSetToImplicitFunction& operator=(const vtkDataSetToImplicitFunction& other);
        vtkDataSetToImplicitFunction(const vtkDataSetToImplicitFunction& other);
        vtkDataSet* m_dataset;
    };

  }
}

#endif



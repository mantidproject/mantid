#ifndef VTKDATASET_TO_GEOMETRY_H_
#define VTKDATASET_TO_GEOMETRY_H_ 

#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/System.h"

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
    class DLLExport vtkDataSetToGeometry
    {

    private:

      vtkDataSet* m_dataSet;

      bool m_executed;

      vtkDataSetToGeometry& operator=(const vtkDataSetToGeometry&);
      
      vtkDataSetToGeometry(const vtkDataSetToGeometry&);

      Mantid::Geometry::IMDDimension_sptr m_xDimension;

      Mantid::Geometry::IMDDimension_sptr m_yDimension;

      Mantid::Geometry::IMDDimension_sptr m_zDimension;

      Mantid::Geometry::IMDDimension_sptr m_tDimension;

      void validate() const;

    public:

      explicit vtkDataSetToGeometry(vtkDataSet* dataSet);

      ~vtkDataSetToGeometry();

      void execute();


      /**
     Getter for x dimension
     @return x dimension.
     */
      Mantid::Geometry::IMDDimension_sptr getXDimension() const
      {
        validate();
        return m_xDimension;
      }

      /**
     Getter for y dimension
     @return y dimension.
     */
      Mantid::Geometry::IMDDimension_sptr getYDimension() const
      {
        validate();
        return m_yDimension;
      }

      /**
     Getter for z dimension
     @return z dimension.
     */
      Mantid::Geometry::IMDDimension_sptr getZDimension() const
      {
        validate();
        return m_zDimension;
      }

     /**
     Getter for t dimension
     @return t dimension.
     */
      Mantid::Geometry::IMDDimension_sptr getTDimension() const
      {
        validate();
        return m_tDimension;
      }


    };
  }
}

#endif
#ifndef VTK_THRESHOLDING_QUAD_FACTORY_H
#define VTK_THRESHOLDING_QUAD_FACTORY_H

#include "MantidKernel/System.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "MantidAPI/IMDWorkspace.h"
#include "vtkUnstructuredGrid.h"

namespace Mantid
{
  namespace VATES
  {

/** Quad Factory. This type is responsible for rendering IMDWorkspaces as surfaces with two spatial dimensions. Ideally this would be done as a polydata type rather than a unstructuredgrid type,
however, some visualisation frameworks won't be able to treat these factories in a covarient fashion. 

 @author Owen Arnold, Tessella plc
 @date 03/05/2011

 Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport vtkThresholdingQuadFactory : public vtkDataSetFactory
    {
    public:

      /// Constructor
      vtkThresholdingQuadFactory(const std::string& scalarName, double minThreshold = -10000, double maxThreshold  = -10000);

      /// Destructor
      virtual ~vtkThresholdingQuadFactory();

      /// Factory Method.
      virtual vtkUnstructuredGrid* create() const;

      virtual vtkUnstructuredGrid* createMeshOnly() const;

      virtual vtkFloatArray* createScalarArray() const;

      virtual void initialize(Mantid::API::IMDWorkspace_sptr);

      typedef std::vector<std::vector<UnstructuredPoint> > Plane;

      typedef std::vector<UnstructuredPoint> Column;

    protected:

      virtual void validate() const;

    private:

      Mantid::API::IMDWorkspace_sptr m_workspace;

      std::string m_scalarName;

      double m_minThreshold;
      
      double m_maxThreshold;
    
    };
    
  }
}
#endif
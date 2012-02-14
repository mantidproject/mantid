#ifndef VTK_THRESHOLDING_LINE_FACTORY_H
#define VTK_THRESHOLDING_LINE_FACTORY_H

#include "MantidKernel/System.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "MantidAPI/IMDWorkspace.h"
#include "vtkUnstructuredGrid.h"
#include "MantidVatesAPI/ThresholdRange.h"
#include "MantidMDEvents/MDHistoWorkspace.h"

namespace Mantid
{
  namespace VATES
  {

/** Line Factory. This type is responsible for rendering IMDWorkspaces as a single dimension.

 @author Owen Arnold, Tessella plc
 @date 09/05/2011

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
    class DLLExport vtkThresholdingLineFactory : public vtkDataSetFactory
    {
    public:

      /// Constructor
      vtkThresholdingLineFactory(ThresholdRange_scptr thresholdRange, const std::string& scalarName);

      /// Assignment operator
      vtkThresholdingLineFactory& operator=(const vtkThresholdingLineFactory& other);

      /// Copy constructor.
      vtkThresholdingLineFactory(const vtkThresholdingLineFactory& other);

      /// Destructor
      virtual ~vtkThresholdingLineFactory();

      /// Factory Method.
      virtual vtkDataSet* create() const;

      virtual void initialize(Mantid::API::Workspace_sptr);

      typedef std::vector<UnstructuredPoint> Column;

      virtual std::string getFactoryTypeName() const
      {
        return "vtkThresholdingLineFactory";
      }

    protected:

      virtual void validate() const;

    private:

      Mantid::MDEvents::MDHistoWorkspace_sptr m_workspace;

      std::string m_scalarName;

      mutable ThresholdRange_scptr m_thresholdRange;
    
    };
    
  }
}
#endif

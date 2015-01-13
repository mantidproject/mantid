#ifndef MANTID_VATES_VTK_MD_HISTO_LINE_FACTORY_H_
#define MANTID_VATES_VTK_MD_HISTO_LINE_FACTORY_H_

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

 Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class DLLExport vtkMDHistoLineFactory : public vtkDataSetFactory
    {
    public:

      /// Constructor
      vtkMDHistoLineFactory(ThresholdRange_scptr thresholdRange, const std::string& scalarName);

      /// Assignment operator
      vtkMDHistoLineFactory& operator=(const vtkMDHistoLineFactory& other);

      /// Copy constructor.
      vtkMDHistoLineFactory(const vtkMDHistoLineFactory& other);

      /// Destructor
      virtual ~vtkMDHistoLineFactory();

      /// Factory Method.
      virtual vtkDataSet* create(ProgressAction& progressUpdating) const;

      virtual void initialize(Mantid::API::Workspace_sptr);

      typedef std::vector<UnstructuredPoint> Column;

      virtual std::string getFactoryTypeName() const
      {
        return "vtkMDHistoLineFactory";
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

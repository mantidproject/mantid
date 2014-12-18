#ifndef VATES_MD_LINE_FACTORY
#define VATES_MD_LINE_FACTORY

#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "MantidVatesAPI/ThresholdRange.h"
#include <boost/shared_ptr.hpp>

namespace Mantid
{
  namespace VATES
  {

   /** Factory for creating a vtkDataSet from an IMDEventWorkspace with a single non-integrated dimensions.
    Delegates processing to a successor if these conditions are not met.
    
    @date 2012-02-10

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class DLLExport vtkMDLineFactory : public vtkDataSetFactory
    {

    public:
      /// Constructor
      vtkMDLineFactory(ThresholdRange_scptr thresholdRange, const std::string& scalarName);

      /// Destructor
      virtual ~vtkMDLineFactory();

      /// Factory Method. Should also handle delegation to successors.
      virtual vtkDataSet* create(ProgressAction& progressUpdating) const;

      /// Initalize with a target workspace.
      virtual void initialize(Mantid::API::Workspace_sptr);

      /// Get the name of the type.
      virtual std::string getFactoryTypeName() const;

    protected:

      /// Template Method pattern to validate the factory before use.
      virtual void validate() const;

    private:

      ///ThresholdRange functor.
      ThresholdRange_scptr m_thresholdRange;

      ///Name of the scalar.
      std::string m_scalarName;

      /// Data source for the visualisation.
      Mantid::API::Workspace_sptr m_workspace;

    };
  }
}

#endif
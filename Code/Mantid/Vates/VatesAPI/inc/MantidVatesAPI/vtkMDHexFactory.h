#ifndef MANTID_VATES_VTK_MD_HEX_FACTORY_H_
#define MANTID_VATES_VTK_MD_HEX_FACTORY_H_

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidVatesAPI/ThresholdRange.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "MantidVatesAPI/TimeToTimeStep.h"
#include <boost/shared_ptr.hpp>

using Mantid::DataObjects::MDEventWorkspace;

namespace Mantid
{
namespace VATES
{

/** Class is used to generate vtkUnstructuredGrids from IMDEventWorkspaces. Utilises the non-uniform nature of the underlying workspace grid/box structure
as the basis for generating visualisation cells. The recursion depth through the box structure is configurable.

 @author Owen Arnold, Tessella plc
 @date 27/July/2011

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

class DLLExport vtkMDHexFactory : public vtkDataSetFactory
{

public:

  /// Constructor
  vtkMDHexFactory(ThresholdRange_scptr thresholdRange, const std::string& scalarName, const size_t maxDepth = 1000);

  /// Destructor
  virtual ~vtkMDHexFactory();

  /// Factory Method. Should also handle delegation to successors.
  virtual vtkDataSet* create(ProgressAction& progressUpdate) const;
  
  /// Initalize with a target workspace.
  virtual void initialize(Mantid::API::Workspace_sptr);

  /// Get the name of the type.
  virtual std::string getFactoryTypeName() const
  {
    return "vtkMDHexFactory";
  }

  virtual void setRecursionDepth(size_t depth);

  /// Set the time value.
  void setTime(double timeStep);

private:

  template<typename MDE, size_t nd>
  void doCreate(typename MDEventWorkspace<MDE, nd>::sptr ws) const;

  /// Template Method pattern to validate the factory before use.
  virtual void validate() const;

  /// Threshold range strategy.
  ThresholdRange_scptr m_thresholdRange;

  /// Scalar name to provide on dataset.
  const std::string m_scalarName;

  /// Member workspace to generate vtkdataset from.
  Mantid::API::Workspace_sptr m_workspace;

  /// Maximum recursion depth to use.
  size_t m_maxDepth;

  /// Data set that will be generated
  mutable vtkDataSet * dataSet;

  /// We are slicing down from > 3 dimensions
  mutable bool slice;

  /// Mask for choosing along which dimensions to slice
  mutable bool * sliceMask;

  /// Implicit function to define which boxes to render.
  mutable Mantid::Geometry::MDImplicitFunction * sliceImplicitFunction;

  /// Time value.
  double m_time;

};


}
}


#endif

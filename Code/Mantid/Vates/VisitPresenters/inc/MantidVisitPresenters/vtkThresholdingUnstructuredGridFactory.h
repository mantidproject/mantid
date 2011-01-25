
#ifndef MANTID_VATES_VTKUNSTRUCTUREDGRIDFACTORY_H_
#define MANTID_VATES_VTKUNSTRUCTUREDGRIDFACTORY_H_

#include <vtkUnstructuredGrid.h>
#include "MantidVisitPresenters/vtkDataSetFactory.h"
#include "MDDataObjects/MDWorkspace.h"

namespace Mantid
{
namespace VATES
{

class vtkThresholdingUnstructuredGridFactory : public vtkDataSetFactory
{
public:

  /// Constructor
  vtkThresholdingUnstructuredGridFactory(Mantid::MDDataObjects::MDWorkspace_sptr workspace, const std::string& scalarname, const int timestep, double threshold=0);

  /// Destructor
  ~vtkThresholdingUnstructuredGridFactory();

  /// Factory method
  vtkUnstructuredGrid* create() const;

private:

  /// MDWorkspace from which to draw.
  Mantid::MDDataObjects::MDWorkspace_sptr m_workspace;

  /// Name of the scalar to provide on mesh.
  const std::string m_scalarName;

  /// timestep obtained from framework.
  const int m_timestep;

  /// Threshold for signal value, below which we do not provide unstructured topologies for.
  const double m_threshold;

};


}
}

#endif

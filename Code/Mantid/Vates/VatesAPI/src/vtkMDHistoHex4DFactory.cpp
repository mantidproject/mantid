#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidVatesAPI/TimeStepToTimeStep.h"
#include "MantidVatesAPI/TimeToTimeStep.h"
#include "MantidVatesAPI/vtkMDHistoHex4DFactory.h"
#include "MantidVatesAPI/ProgressAction.h"
#include <boost/math/special_functions/fpclassify.hpp>

using Mantid::API::IMDWorkspace;
using Mantid::Kernel::CPUTimer;
using namespace Mantid::DataObjects;

namespace Mantid
{
namespace VATES
{

  template<typename TimeMapper>
  vtkMDHistoHex4DFactory<TimeMapper>::vtkMDHistoHex4DFactory(ThresholdRange_scptr thresholdRange, const std::string& scalarName, const double timestep)
  : vtkMDHistoHexFactory(thresholdRange,  scalarName),
    m_timestep(timestep)
  {
  }

    /**
  Assigment operator
  @param other : vtkMDHistoHex4DFactory to assign to this instance from.
  @return ref to assigned current instance.
  */
  template<typename TimeMapper>
  vtkMDHistoHex4DFactory<TimeMapper>& vtkMDHistoHex4DFactory<TimeMapper>::operator=(const vtkMDHistoHex4DFactory<TimeMapper>& other)
  {
    if(this != &other)
    {
      this->m_scalarName = other.m_scalarName;
      this->m_thresholdRange = other.m_thresholdRange;
      this->m_workspace = other.m_workspace;
      this->m_timestep = other.m_timestep;
      this->m_timeMapper = other.m_timeMapper;
    }
    return *this;
  }

  /**
  Copy Constructor
  @param other : instance to copy from.
  */
  template<typename TimeMapper>
  vtkMDHistoHex4DFactory<TimeMapper>::vtkMDHistoHex4DFactory(const vtkMDHistoHex4DFactory<TimeMapper>& other)
   : vtkMDHistoHexFactory(other),
     m_timestep(other.m_timestep), m_timeMapper(other.m_timeMapper)
  {
    
  }


  template<typename TimeMapper>
  void vtkMDHistoHex4DFactory<TimeMapper>::initialize(Mantid::API::Workspace_sptr workspace)
  {
    m_workspace = doInitialize<MDHistoWorkspace, 4>(workspace);
    if(m_workspace != NULL)
    {
      double tMax = m_workspace->getTDimension()->getMaximum();
      double tMin = m_workspace->getTDimension()->getMinimum();
      size_t nbins = m_workspace->getTDimension()->getNBins();

      m_timeMapper = TimeMapper::construct(tMin, tMax, nbins);

      //Setup range values according to whatever strategy object has been injected.
      m_thresholdRange->setWorkspace(workspace);
      m_thresholdRange->calculate();
    }
  }

  template<typename TimeMapper>
  void vtkMDHistoHex4DFactory<TimeMapper>::validate() const
  {
    validateWsNotNull();
  }

  /**
  Create the vtkStructuredGrid from the provided workspace
  @param progressUpdating: Reporting object to pass progress information up the stack.
  @return fully constructed vtkDataSet.
  */
  template<typename TimeMapper>
  vtkDataSet* vtkMDHistoHex4DFactory<TimeMapper>::create(ProgressAction& progressUpdating) const
  {
    vtkDataSet* product = tryDelegatingCreation<MDHistoWorkspace, 4>(m_workspace, progressUpdating);
    if(product != NULL)
    {
      return product;
    }
    else
    {
      // Create the mesh in a 4D mode
      return this->create3Dor4D(m_timeMapper(m_timestep), true, progressUpdating);
    }
  }


  template<typename TimeMapper>
  vtkMDHistoHex4DFactory<TimeMapper>::~vtkMDHistoHex4DFactory()
  {
  }

  template class vtkMDHistoHex4DFactory<TimeToTimeStep>;
  template class vtkMDHistoHex4DFactory<TimeStepToTimeStep>;

}
}

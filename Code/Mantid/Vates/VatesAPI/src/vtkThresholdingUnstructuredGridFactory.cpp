#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidVatesAPI/TimeStepToTimeStep.h"
#include "MantidVatesAPI/TimeToTimeStep.h"
#include "MantidVatesAPI/vtkThresholdingUnstructuredGridFactory.h"
#include <boost/math/special_functions/fpclassify.hpp>

using Mantid::API::IMDWorkspace;
using Mantid::Kernel::CPUTimer;
using namespace Mantid::MDEvents;

namespace Mantid
{
namespace VATES
{

  template<typename TimeMapper>
  vtkThresholdingUnstructuredGridFactory<TimeMapper>::vtkThresholdingUnstructuredGridFactory(ThresholdRange_scptr thresholdRange, const std::string& scalarName, const double timestep)
  : vtkThresholdingHexahedronFactory(thresholdRange,  scalarName),
    m_timestep(timestep)
  {
  }

    /**
  Assigment operator
  @param other : vtkThresholdingHexahedronFactory to assign to this instance from.
  @return ref to assigned current instance.
  */
  template<typename TimeMapper>
  vtkThresholdingUnstructuredGridFactory<TimeMapper>& vtkThresholdingUnstructuredGridFactory<TimeMapper>::operator=(const vtkThresholdingUnstructuredGridFactory<TimeMapper>& other)
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
  vtkThresholdingUnstructuredGridFactory<TimeMapper>::vtkThresholdingUnstructuredGridFactory(const vtkThresholdingUnstructuredGridFactory<TimeMapper>& other)
   : vtkThresholdingHexahedronFactory(other),
     m_timestep(other.m_timestep), m_timeMapper(other.m_timeMapper)
  {
    
  }


  template<typename TimeMapper>
  void vtkThresholdingUnstructuredGridFactory<TimeMapper>::initialize(Mantid::API::Workspace_sptr workspace)
  {
    m_workspace = boost::dynamic_pointer_cast<MDHistoWorkspace>(workspace);
    // Check that a workspace has been provided.
    validateWsNotNull();
    // When the workspace can not be handled by this type, take action in the form of delegation.
    size_t nonIntegratedSize = m_workspace->getNonIntegratedDimensions().size();
    if((doesCheckDimensionality() && nonIntegratedSize != vtkDataSetFactory::FourDimensional))
    {
      if(this->hasSuccessor())
      {
        m_successor->initialize(m_workspace);
        return;
      }
      else
      {
        throw std::runtime_error("There is no successor factory set for this vtkThresholdingUnstructuredGridFactory type");
      }
    }

    double tMax = m_workspace->getTDimension()->getMaximum();
    double tMin = m_workspace->getTDimension()->getMinimum();
    size_t nbins = m_workspace->getTDimension()->getNBins();

    m_timeMapper = TimeMapper::construct(tMin, tMax, nbins);

    //Setup range values according to whatever strategy object has been injected.
    m_thresholdRange->setWorkspace(m_workspace);
    m_thresholdRange->calculate();
  }

  template<typename TimeMapper>
  void vtkThresholdingUnstructuredGridFactory<TimeMapper>::validate() const
  {
    validateWsNotNull();
  }

  template<typename TimeMapper>
  vtkDataSet* vtkThresholdingUnstructuredGridFactory<TimeMapper>::create() const
  {
    validate();

    size_t nonIntegratedSize = m_workspace->getNonIntegratedDimensions().size();
    if((doesCheckDimensionality() && nonIntegratedSize != vtkDataSetFactory::FourDimensional))
    {
      return m_successor->create();
    }
    else
    { 
      // Create the mesh in a 4D mode
      return this->create3Dor4D(m_timeMapper(m_timestep), true);
    }
  }


  template<typename TimeMapper>
  vtkThresholdingUnstructuredGridFactory<TimeMapper>::~vtkThresholdingUnstructuredGridFactory()
  {
  }

  template<typename TimeMapper>
  vtkDataSet* vtkThresholdingUnstructuredGridFactory<TimeMapper>::createMeshOnly() const
  {
    throw std::runtime_error("::createMeshOnly() does not apply for this type of factory.");
  }

  template<typename TimeMapper>
  vtkFloatArray* vtkThresholdingUnstructuredGridFactory<TimeMapper>::createScalarArray() const
  {
    throw std::runtime_error("::createScalarArray() does not apply for this type of factory.");
  }



  template class vtkThresholdingUnstructuredGridFactory<TimeToTimeStep>;
  template class vtkThresholdingUnstructuredGridFactory<TimeStepToTimeStep>;

}
}

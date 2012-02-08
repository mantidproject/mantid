#ifndef VATES_API_GENERIC_IMD_FACTORY
#define VATES_API_GENERIC_IMD_FACTORY

#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "MantidVatesAPI/ThresholdRange.h"
#include <boost/shared_ptr.hpp>

namespace Mantid
{
  namespace VATES
  {
    class DLLExport vtkMDQuadFactory : public vtkDataSetFactory
    {

    public:
      /// Constructor
      vtkMDQuadFactory(ThresholdRange_scptr thresholdRange, const std::string& scalarName);

      /// Destructor
      virtual ~vtkMDQuadFactory();

      /// Factory Method. Should also handle delegation to successors.
      virtual vtkDataSet* create() const;

      /// Create as a mesh only.
      virtual vtkDataSet* createMeshOnly() const;

      /// Create the scalar array only.
      virtual vtkFloatArray* createScalarArray() const;

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
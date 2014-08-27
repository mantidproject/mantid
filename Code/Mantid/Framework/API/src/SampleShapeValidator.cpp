//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidAPI/SampleShapeValidator.h"
#include "MantidGeometry/Objects/Object.h"

namespace Mantid
{
  namespace API
  {
    //-----------------------------------------------------------------------------
    // Public methods
    //-----------------------------------------------------------------------------

    /// @copydoc TypedValidator::getType
    std::string SampleShapeValidator::getType() const
    {
      return "SampleShape";
    }

    /// @copydoc TypedValidator::clone
    Kernel::IValidator_sptr SampleShapeValidator::clone() const
    {
      return boost::make_shared<SampleShapeValidator>();
    }

    //-----------------------------------------------------------------------------
    // Private methods
    //-----------------------------------------------------------------------------

    /**
     * Checks that the workspace has a valid sample shape defined
     *  @param value :: The workspace to test
     *  @return A user level description if a problem exists or ""
     */
    std::string SampleShapeValidator::checkValidity( const boost::shared_ptr<ExperimentInfo>& value ) const
    {
      const auto & sampleShape = value->sample().getShape();
      if(sampleShape.hasValidShape())
      {
        return "";
      }
      else
      {
        return "Invalid or no shape defined for sample";
      }
    }

  } // namespace API
} // namespace Mantid

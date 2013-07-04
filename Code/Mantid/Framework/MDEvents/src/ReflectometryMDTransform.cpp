#include "MantidMDEvents/ReflectometryMDTransform.h"

using namespace Mantid::API;

namespace Mantid
{
  namespace MDEvents
  {

    ReflectometryMDTransform::ReflectometryMDTransform() :
        m_nbinsx(10), m_nbinsz(10)
    {
    }

    ReflectometryMDTransform::~ReflectometryMDTransform()
    {
    }

    boost::shared_ptr<MDEventWorkspace2Lean> ReflectometryMDTransform::createWorkspace(
        Mantid::Geometry::IMDDimension_sptr a, Mantid::Geometry::IMDDimension_sptr b) const
    {
      auto ws = boost::make_shared<MDEventWorkspace2Lean>();

      ws->addDimension(a);
      ws->addDimension(b);

      // Set some reasonable values for the box controller
      BoxController_sptr bc = ws->getBoxController();
      bc->setSplitInto(2);
      bc->setSplitThreshold(10);

      // Initialize the workspace.
      ws->initialize();

      // Start with a MDGridBox.
      ws->splitBox();
      return ws;
    }

  }
}

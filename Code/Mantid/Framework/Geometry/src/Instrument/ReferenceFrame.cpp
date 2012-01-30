#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;

namespace Mantid
{
namespace Geometry
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
  @param up : pointing up axis
  @param alongBeam : axis pointing along the beam
  @param handedness : Handedness
  @param origin : origin
  */
  ReferenceFrame::ReferenceFrame(PointingAlong up, PointingAlong alongBeam, Handedness handedness, std::string origin) 
    : m_up(up), m_alongBeam(alongBeam), m_handedness(handedness), m_origin(origin)
  {
    if(up == alongBeam)
    {
      throw std::invalid_argument("Cannot have up direction the same as the beam direction");
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor
  Default constructor
  */
  ReferenceFrame::ReferenceFrame()
    : m_up(Y), m_alongBeam(Z), m_handedness(Right), m_origin("source")
  {
  }


  //----------------------------------------------------------------------------------------------
  /** Destructor
  */
  ReferenceFrame::~ReferenceFrame()
  {
  }

  /**
  Gets the pointing up direction
  @return axis
  */
  PointingAlong ReferenceFrame::pointingUp() const
  {
    return m_up;
  }

  /** Gets the beam pointing along direction
  @return axis
  */
  PointingAlong ReferenceFrame::pointingAlongBeam() const
  {
    return m_alongBeam;
  }

  /** Gets the handedness
  @return handedness
  */
  Handedness ReferenceFrame::getHandedness() const
  {
    return m_handedness;
  }

  /** Gets the origin
  @return origin
  */
  std::string ReferenceFrame::origin() const
  {
    return m_origin;
  }


} // namespace Mantid
} // namespace Geometry
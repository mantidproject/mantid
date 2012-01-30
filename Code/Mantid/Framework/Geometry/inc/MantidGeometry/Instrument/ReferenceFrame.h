#ifndef MANTID_GEOMETRY_REFERENCEFRAME_H_
#define MANTID_GEOMETRY_REFERENCEFRAME_H_

#include "MantidKernel/System.h"


namespace Mantid
{
namespace Geometry
{
  ///Type to describe pointing along options
  enum PointingAlong{X, Y, Z};
  ///Type to distingusih between l and r handedness
  enum Handedness{Left, Right};

  /** ReferenceFrame : Holds reference frame information from the geometry description file.
    
    @date 2012-01-27

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  
  class DLLExport ReferenceFrame 
  {
  public:
    /// Default constructor
    ReferenceFrame();
    /// Constructor
    ReferenceFrame(PointingAlong up, PointingAlong alongBeam, Handedness handedNess, std::string origin);
    /// Gets the pointing up direction
    PointingAlong pointingUp() const;
    /// Gets the beam pointing along direction
    PointingAlong pointingAlongBeam() const;
    /// Gets the handedness
    Handedness getHandedness() const;
    /// Gets the origin
    std::string origin() const;
    /// Destructor
    virtual ~ReferenceFrame();

  private:
    ///Disabled assignment
    ReferenceFrame& operator=(const ReferenceFrame&);
    ///Disabled copy construction
    ReferenceFrame(const ReferenceFrame&);

    /// Pointing up axis
    PointingAlong m_up;
    /// Beam pointing along axis
    PointingAlong m_alongBeam;
    /// Handedness
    Handedness m_handedness;
    /// Origin 
    std::string m_origin;

  };


} // namespace Geometry
} // namespace Mantid

#endif  /* MANTID_GEOMETRY_REFERENCEFRAME_H_ */
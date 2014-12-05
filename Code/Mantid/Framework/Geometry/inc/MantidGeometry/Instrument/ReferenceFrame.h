#ifndef MANTID_GEOMETRY_REFERENCEFRAME_H_
#define MANTID_GEOMETRY_REFERENCEFRAME_H_

#include "MantidKernel/V3D.h"
#include "MantidKernel/System.h"
#include <string>


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
    /// Gets the pointing horizontal direction, i.e perpendicular to up & along beam
    PointingAlong pointingHorizontal() const;
    /// Gets the handedness
    Handedness getHandedness() const;
    /// Gets the origin
    std::string origin() const;
    /// Destructor
    virtual ~ReferenceFrame();
    /// Convert up axis into a 3D direction
    const Mantid::Kernel::V3D vecPointingUp() const;
    /// Convert along beam axis into a 3D direction
    const Mantid::Kernel::V3D vecPointingAlongBeam() const;
    /// Pointing up axis as a string
    std::string pointingUpAxis() const;
    /// Pointing along beam axis as a string
    std::string pointingAlongBeamAxis() const;
    /// Pointing horizontal to beam as a string
    std::string pointingHorizontalAxis() const;
  private:
    /// Common setup
    void init();
    ///Disabled assignment
    ReferenceFrame& operator=(const ReferenceFrame&);
    /// Pointing up axis
    PointingAlong m_up;
    /// Beam pointing along axis
    PointingAlong m_alongBeam;
    /// Handedness
    Handedness m_handedness;
    /// Origin 
    std::string m_origin;
    /// Vector pointing along the beam
    Mantid::Kernel::V3D m_vecPointingAlongBeam;
    /// Vector pointing up instrument
    Mantid::Kernel::V3D m_vecPointingUp;

  };


} // namespace Geometry
} // namespace Mantid

#endif  /* MANTID_GEOMETRY_REFERENCEFRAME_H_ */

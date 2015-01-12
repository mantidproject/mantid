#ifndef MANTID_DATAOBJECTS_PEAKSHAPESPHERICAL_H_
#define MANTID_DATAOBJECTS_PEAKSHAPESPHERICAL_H_

#include "MantidKernel/System.h"
#include "MantidKernel/VMD.h"
#include "MantidDataObjects/PeakShape.h"
#include "MantidAPI/SpecialCoordinateSystem.h"


namespace Mantid
{
namespace DataObjects
{

  /** PeakShapeSpherical : PeakShape for a spherical peak

    Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class DLLExport PeakShapeSpherical : public PeakShape
  {
  public:
    /// Constructor
    PeakShapeSpherical(const Mantid::Kernel::VMD& peakCentre, const double& peakRadius, API::SpecialCoordinateSystem frame, std::string algorithmName = std::string(), int algorithmVersion = -1);
    /// Destructor
    virtual ~PeakShapeSpherical();
    /// Copy constructor
    PeakShapeSpherical(const PeakShapeSpherical& other);
    /// Assignment operator
    PeakShapeSpherical& operator=(const PeakShapeSpherical& other);
    /// Creation coordinate frame (coordinate frame used to create shape)
    virtual API::SpecialCoordinateSystem frame() const;
    /// Serialization method
    virtual std::string toJSON() const;
    /// Peak algorithm name
    virtual std::string algorithmName() const;
    /// Peak algorithm version
    virtual int algorithmVersion() const;
    /// Clone the peak shape
    virtual PeakShapeSpherical* clone() const;

    /// Peak centre
    Mantid::Kernel::VMD centre() const;
    /// Peak radius
    double radius() const;



  private:
    /// Peak radius
    double m_radius;
    /// Peak centre
    Mantid::Kernel::VMD m_centre;
    /// Special coordinate system
    Mantid::API::SpecialCoordinateSystem m_frame;
    /// Generating algorithm name
    std::string m_algorithmName;
    /// Generating algorithm version
    int m_algorithmVersion;

  };


} // namespace DataObjects
} // namespace Mantid

#endif  /* MANTID_DATAOBJECTS_PEAKSHAPESPHERICAL_H_ */

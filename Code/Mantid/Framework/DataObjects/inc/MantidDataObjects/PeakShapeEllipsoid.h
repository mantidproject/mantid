#ifndef MANTID_DATAOBJECTS_PEAKSHAPEELLIPSOID_H_
#define MANTID_DATAOBJECTS_PEAKSHAPEELLIPSOID_H_

#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"
#include "MantidDataObjects/PeakShapeBase.h"


namespace Mantid
{
namespace DataObjects
{

  /** PeakShapeEllipsoid : PeakShape representing a 3D ellipsoid

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
  class DLLExport PeakShapeEllipsoid : public PeakShapeBase
  {
  public:
    /// Constructor
    PeakShapeEllipsoid(std::vector<Mantid::Kernel::V3D> directions, std::vector<double> abcRadius, std::vector<double> abcBackgroundInnerRadius, std::vector<double>  abcBackgroundOuterRadius, API::SpecialCoordinateSystem frame,
                       std::string algorithmName = std::string(),
                       int algorithmVersion = -1);
    /// Copy constructor
    PeakShapeEllipsoid(const PeakShapeEllipsoid& other);
    /// Assignment operator
    PeakShapeEllipsoid& operator=(const PeakShapeEllipsoid& other);
    /// Equals operator
    bool operator==(const PeakShapeEllipsoid& other) const;
    /// Destructor
    virtual ~PeakShapeEllipsoid();
    /// Get radii
    std::vector<double> abcRadii() const;
    /// Get background inner radii
    std::vector<double> abcRadiiBackgroundInner() const;
    /// Get background outer radii
    std::vector<double> abcRadiiBackgroundOuter() const;
    /// Get ellipsoid directions
    std::vector<Mantid::Kernel::V3D> directions() const;
    
    /// PeakShape interface
    std::string toJSON() const;
    /// Clone ellipsoid
    PeakShapeEllipsoid *clone() const;
    /// Get the peak shape
    std::string shapeName() const;

    static const std::string ellipsoidShapeName();

  private:
    /// principle axis
    std::vector<Mantid::Kernel::V3D> m_directions;
    /// radii
    std::vector<double> m_abc_radii;
    /// inner radii
    std::vector<double> m_abc_radiiBackgroundInner;
    /// outer radii
    std::vector<double> m_abc_radiiBackgroundOuter;


  };

  typedef boost::shared_ptr<PeakShapeEllipsoid> PeakShapeEllipsoid_sptr;
  typedef boost::shared_ptr<const PeakShapeEllipsoid> PeakShapeEllipsoid_const_sptr;


} // namespace DataObjects
} // namespace Mantid

#endif  /* MANTID_DATAOBJECTS_PEAKSHAPEELLIPSOID_H_ */

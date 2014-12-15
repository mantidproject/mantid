#ifndef MANTID_CRYSTAL_PEAKSINREGION_H_
#define MANTID_CRYSTAL_PEAKSINREGION_H_

#include "MantidCrystal/PeaksIntersection.h"


namespace Mantid
{
namespace Crystal
{

  /** PeaksInRegion : Find peaks that are either inside a box region, or that have a radius of sufficent size, that they intersect the box.
    
    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class DLLExport PeaksInRegion  : public PeaksIntersection
  {
  public:
    PeaksInRegion();
    virtual ~PeaksInRegion();
    
    virtual const std::string name() const;
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Find peaks intersecting a box region.";}

    virtual int version() const;
    virtual const std::string category() const;


  private:
    void init();
    void exec();

    // Overriden base class methods.
    virtual void validateExtentsInput() const;
    virtual int numberOfFaces() const;
    virtual VecVecV3D createFaces() const;
    virtual bool pointOutsideAnyExtents(const Mantid::Kernel::V3D& testPoint) const;
    virtual bool pointInsideAllExtents(const Mantid::Kernel::V3D& testPoints, const Mantid::Kernel::V3D& peakCenter) const; 
    virtual void checkTouchPoint(const Mantid::Kernel::V3D& touchPoint,const Mantid::Kernel::V3D& normal,const  Mantid::Kernel::V3D& faceVertex) const;

    /// Extents.
    std::vector<double> m_extents;

  };


} // namespace Crystal
} // namespace Mantid

#endif  /* MANTID_CRYSTAL_PEAKSINREGION_H_ */

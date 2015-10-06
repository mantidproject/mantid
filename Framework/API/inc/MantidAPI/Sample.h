#ifndef MANTID_API_SAMPLE_H_
#define MANTID_API_SAMPLE_H_

//-----------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidKernel/V3D.h"
#include <vector>

namespace Mantid {
//-----------------------------------------------------------------------------
// Geometry forward declarations
//------------------------------------------------------------------------------
namespace Geometry {
class OrientedLattice;
}

namespace API {
//-----------------------------------------------------------------------------
// API forward declarations
//------------------------------------------------------------------------------
class SampleEnvironment;

/**
  This class stores information about the sample used in particular
  run. It is a type of ObjComponent meaning it has a shape, a position
  and a material.

  Copyright &copy; 2007-2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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

  File change history is stored at: <https://github.com/mantidproject/mantid>.
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_API_DLL Sample {
public:
  Sample();
  Sample(const Sample &copy);
  ~Sample();
  Sample &operator=(const Sample &rhs);

  void saveNexus(::NeXus::File *file, const std::string &group) const;
  int loadNexus(::NeXus::File *file, const std::string &group);

  /// index operator for accessing multiple samples
  Sample &operator[](const int index);
  /// the number of samples
  std::size_t size() const;
  /// Adds a sample to the list
  void addSample(boost::shared_ptr<Sample> childSample);

  /// Returns the name of the sample
  const std::string &getName() const;
  /// Set the name of the sample
  void setName(const std::string &name);
  /// Return the sample shape
  const Geometry::Object &getShape() const;
  /// Update the shape of the object
  void setShape(const Geometry::Object &shape);

  /** @name Material properties.*/
  //@{
  /// Return the material (convenience method)
  const Kernel::Material &getMaterial() const;
  //@}

  /** @name Access the environment information */
  //@{
  /// Get a reference to the sample's environment
  const SampleEnvironment &getEnvironment() const;
  /// Set the environment used to contain the sample
  void setEnvironment(SampleEnvironment *env);
  //@}

  /** @name Access the sample's lattice structure and orientation */
  //@{
  /// Get a reference to the sample's OrientedLattice
  const Geometry::OrientedLattice &getOrientedLattice() const;
  /// Get a reference to the sample's OrientedLattice
  Geometry::OrientedLattice &getOrientedLattice();
  /** Set the pointer to OrientedLattice defining the sample's lattice and
     orientation.
      No copying is done in the class, but the class deletes pointer on
     destruction so the application, providing the pointer should not do it*/
  void setOrientedLattice(Geometry::OrientedLattice *latt);
  bool hasOrientedLattice() const;
  //@}

  // Required for SANS work until we define a proper
  // sample object from the raw file information
  /**@name Legacy functions */
  //@{
  /// Sets the geometry flag
  void setGeometryFlag(int geom_id);
  /// Returns the geometry flag
  int getGeometryFlag() const;
  /// Sets the thickness
  void setThickness(double thick);
  /// Returns the thickness
  double getThickness() const;
  /// Sets the height
  void setHeight(double height);
  /// Returns the height
  double getHeight() const;
  /// Sets the width
  void setWidth(double width);
  /// Returns the width
  double getWidth() const;
  //@}
  /// Delete the oriented lattice
  void clearOrientedLattice();

private:
  /// The sample name
  std::string m_name;
  /// The sample shape object
  Geometry::Object m_shape;
  /// An owned pointer to the SampleEnvironment object
  boost::shared_ptr<SampleEnvironment> m_environment;
  /// Pointer to the OrientedLattice of the sample, NULL if not set.
  Geometry::OrientedLattice *m_lattice;

  /// Vector of child samples
  std::vector<boost::shared_ptr<Sample>> m_samples;

  /// The sample geometry flag
  int m_geom_id;
  /// The sample thickness from the SPB_STRUCT in the raw file
  double m_thick;
  /// The sample height from the SPB_STRUCT in the raw file
  double m_height;
  /// The sample width from the SPB_STRUCT in the raw file
  double m_width;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_SAMPLE_H_*/

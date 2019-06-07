// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_SAMPLE_H_
#define MANTID_API_SAMPLE_H_

//-----------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
//-----------------------------------------------------------------------------
// Geometry forward declarations
//------------------------------------------------------------------------------
namespace Geometry {
class CrystalStructure;
class OrientedLattice;
class SampleEnvironment;
} // namespace Geometry

namespace API {

/**
  This class stores information about the sample used in particular
  run. It is a type of ObjComponent meaning it has a shape, a position
  and a material.
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
  const Geometry::IObject &getShape() const;
  /// Update the shape of the object
  void setShape(const Geometry::IObject_sptr &shape);

  /** @name Material properties.*/
  //@{
  /// Return the material (convenience method)
  const Kernel::Material &getMaterial() const;
  //@}

  /** @name Access the environment information */
  //@{
  bool hasEnvironment() const;
  /// Get a reference to the sample's environment
  const Geometry::SampleEnvironment &getEnvironment() const;
  /// Set the environment used to contain the sample
  void setEnvironment(std::unique_ptr<Geometry::SampleEnvironment> env);
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

  /** @name Access the sample's crystal structure */
  //@{
  const Geometry::CrystalStructure &getCrystalStructure() const;
  void
  setCrystalStructure(const Geometry::CrystalStructure &newCrystalStructure);
  bool hasCrystalStructure() const;
  void clearCrystalStructure();
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
  Geometry::IObject_sptr m_shape;
  /// An owned pointer to the SampleEnvironment object
  boost::shared_ptr<Geometry::SampleEnvironment> m_environment;
  /// Pointer to the OrientedLattice of the sample, NULL if not set.
  std::unique_ptr<Geometry::OrientedLattice> m_lattice;

  /// CrystalStructure of the sample
  std::unique_ptr<Geometry::CrystalStructure> m_crystalStructure;

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

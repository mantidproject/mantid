// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/Sample.h"
#include "MantidGeometry/Crystal/CrystalStructure.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/Strings.h"
#include "MantidNexusCpp/NeXusException.hpp"

#include <utility>

namespace Mantid::API {
using namespace Mantid::Kernel;
using Geometry::IObject;
using Geometry::IObject_sptr;
using Geometry::OrientedLattice;
using Geometry::SampleEnvironment;
using Geometry::ShapeFactory;

/**
 * Default constructor. Required for cow_ptr.
 */
Sample::Sample()
    : m_name(), m_shape(ShapeFactory().createShape("")), m_environment(), m_lattice(nullptr), m_crystalStructure(),
      m_samples(), m_geom_id(0), m_thick(0.0), m_height(0.0), m_width(0.0) {}

/**
 * Copy constructor
 *  @param copy :: const reference to the sample object
 */
Sample::Sample(const Sample &copy)
    : m_name(copy.m_name), m_shape(copy.m_shape), m_environment(copy.m_environment), m_lattice(nullptr),
      m_crystalStructure(), m_samples(copy.m_samples), m_geom_id(copy.m_geom_id), m_thick(copy.m_thick),
      m_height(copy.m_height), m_width(copy.m_width) {
  if (copy.m_lattice)
    m_lattice = std::make_unique<OrientedLattice>(copy.getOrientedLattice());

  if (copy.hasCrystalStructure()) {
    m_crystalStructure = std::make_unique<Geometry::CrystalStructure>(copy.getCrystalStructure());
  }
}

/// Destructor
Sample::~Sample() = default;

/** Assignment operator
 * @param rhs :: const reference to the sample object
 * @return A reference to this object, which will have the same
 * state as the argument
 */
Sample &Sample::operator=(const Sample &rhs) {
  if (this == &rhs)
    return *this;
  m_name = rhs.m_name;
  m_shape = rhs.m_shape;
  m_environment = rhs.m_environment;
  m_geom_id = rhs.m_geom_id;
  m_samples = std::vector<std::shared_ptr<Sample>>(rhs.m_samples);
  m_thick = rhs.m_thick;
  m_height = rhs.m_height;
  m_width = rhs.m_width;
  if (rhs.m_lattice)
    m_lattice = std::make_unique<OrientedLattice>(rhs.getOrientedLattice());
  else
    m_lattice.reset(nullptr);

  m_crystalStructure.reset();
  if (rhs.hasCrystalStructure()) {
    m_crystalStructure.reset(new Geometry::CrystalStructure(rhs.getCrystalStructure()));
  }

  return *this;
}

/**
 * Returns the name of the sample
 * @returns The name of this  sample
 */
const std::string &Sample::getName() const { return m_name; }

/**
 * Update the name of the sample
 * @param name :: The name of the sample
 */
void Sample::setName(const std::string &name) { m_name = name; }

bool Sample::hasShape() const { return (m_shape != nullptr); }

/**
 * Get a reference to the sample shape object. It is assumed that this is defined
 * within
 * its own coordinate system with its centre at [0,0,0]
 * @return A reference to the object describing the shape
 */
const IObject &Sample::getShape() const { return *m_shape; }

/**
 * Get a pointer to the sample shape object. It is assumed that this is defined
 * within
 * its own coordinate system with its centre at [0,0,0]
 * @return A pointer to the object describing the shape
 */
const IObject_sptr Sample::getShapePtr() const { return m_shape; }

/** Set the object that describes the sample shape. The object is defined within
 * its own coordinate system
 * @param shape :: The object describing the shape
 */
void Sample::setShape(const IObject_sptr &shape) {
  if (shape) {
    m_shape = shape;
  } else {
    m_shape = ShapeFactory().createShape("");
  }
}

/** Return the material.
 * @return A reference to the material the sample is composed of
 */
const Material &Sample::getMaterial() const { return m_shape->material(); }

bool Sample::hasEnvironment() const { return (m_environment != nullptr); }

/**
 * Return a reference to the sample environment that this sample is attached to
 * @return A const reference to a SampleEnvironment object
 * @throw std::runtime_error If the environment has not been defined
 */
const SampleEnvironment &Sample::getEnvironment() const {
  if (!m_environment) {
    throw std::runtime_error("Sample::getEnvironment - No sample enviroment has been defined.");
  }
  return *m_environment;
}

/**
 * Attach an environment onto this sample
 * @param env :: A pointer to a created sample environment. This takes
 * ownership of the object.
 */
void Sample::setEnvironment(std::shared_ptr<SampleEnvironment> env) { m_environment = std::move(env); }

/** Return a const reference to the OrientedLattice of this sample
 * @return A const reference to a OrientedLattice object
 * @throw std::runtime_error If the OrientedLattice has not been defined
 */
const OrientedLattice &Sample::getOrientedLattice() const {
  if (!m_lattice) {
    throw std::runtime_error("Sample::getOrientedLattice - No OrientedLattice has been defined.");
  }
  return *m_lattice;
}

/** Return a reference to the OrientedLattice of this sample
 * @return A reference to a OrientedLattice object
 * @throw std::runtime_error If the OrientedLattice has not been defined
 */
OrientedLattice &Sample::getOrientedLattice() {
  if (!m_lattice) {
    throw std::runtime_error("Sample::getOrientedLattice - No OrientedLattice has been defined.");
  }
  return *m_lattice;
}

/** Attach an OrientedLattice onto this sample
 *
 * @param lattice :: A pointer to a OrientedLattice.
 */
void Sample::setOrientedLattice(std::unique_ptr<Geometry::OrientedLattice> lattice) { m_lattice = std::move(lattice); }

/** @return true if the sample has an OrientedLattice  */
bool Sample::hasOrientedLattice() const { return (m_lattice != nullptr); }

const Geometry::CrystalStructure &Sample::getCrystalStructure() const {
  if (!hasCrystalStructure()) {
    throw std::runtime_error("Sample::getCrystalStructure - No CrystalStructure has been defined.");
  }

  return *m_crystalStructure;
}

/// Resets the internal pointer to the new CrystalStructure (it's copied).
void Sample::setCrystalStructure(const Geometry::CrystalStructure &newCrystalStructure) {
  m_crystalStructure.reset(new Geometry::CrystalStructure(newCrystalStructure));
}

/// Returns true if the sample actually holds a CrystalStructure.
bool Sample::hasCrystalStructure() const {
  // Conversion to bool seems to be a problem in VS2012, so this is a bit more
  // verbose than it should be.
  return static_cast<bool>(m_crystalStructure);
}

/// Destroys the internally stored CrystalStructure-object.
void Sample::clearCrystalStructure() { m_crystalStructure.reset(); }

/**
 * Set the geometry flag that is specfied in the raw file within the SPB_STRUCT
 * 1 = cylinder, 2 = flat plate, 3 = disc, 4 = single crystal
 * @param geom_id :: The flag for the geometry
 */
void Sample::setGeometryFlag(int geom_id) { m_geom_id = geom_id; }

/**
 * Get the geometry flag that is specified in the raw file within the SPB_STRUCT
 * 1 = cylinder, 2 = flat plate, 3 = disc, 4 = single crystal
 * @returns The flag for the sample geometry
 */
int Sample::getGeometryFlag() const { return m_geom_id; }

/**
 * Set the thickness value
 * @param thick :: The parameter e_thick in the SPB_STRUCT
 */
void Sample::setThickness(double thick) { m_thick = thick; }

/**
 * Get the thickness value
 * @returns The parameter thickness parameter
 */
double Sample::getThickness() const { return m_thick; }

/**
 * Set the height value
 * @param height :: The parameter e_height in the SPB_STRUCT
 */
void Sample::setHeight(double height) { m_height = height; }

/**
 * Get the height value
 * @returns The parameter height parameter
 */
double Sample::getHeight() const { return m_height; }

/**
 * Set the width value
 * @param width :: The parameter e_width in the SPB_STRUCT
 */
void Sample::setWidth(double width) { m_width = width; }

/**
 * Get the height value
 * @returns The parameter height parameter
 */
double Sample::getWidth() const { return m_width; }

/**
 * Gets the desired sample, 0 is the current sample
 * @param index The index of the desired sample
 * @returns The desired sample
 */
Sample &Sample::operator[](const int index) {
  if (index == 0) {
    return *this;
  } else if ((static_cast<std::size_t>(index) > m_samples.size()) || (index < 0)) {
    throw std::out_of_range("The index value provided was out of range");
  } else {
    return *m_samples[index - 1];
  }
}

/**
 * Gets the number of samples in this collection
 * @returns The count of samples
 */
std::size_t Sample::size() const { return m_samples.size() + 1; }

/**
 * Adds a sample to the sample collection
 * @param childSample The child sample to be added
 */
void Sample::addSample(const std::shared_ptr<Sample> &childSample) { m_samples.emplace_back(childSample); }

//--------------------------------------------------------------------------------------------
/** Save the object to an open NeXus file.
 * @param file :: open NeXus file
 * @param group :: name of the group to create
 */
void Sample::saveNexus(::NeXus::File *file, const std::string &group) const {
  file->makeGroup(group, "NXsample", true);
  file->putAttr("name", m_name);
  if (m_name.empty()) {
    file->putAttr("name_empty", 1);
  }
  file->putAttr("version", 1);
  std::string shapeXML("");
  if (auto csgObject = std::dynamic_pointer_cast<Mantid::Geometry::CSGObject>(m_shape)) {
    shapeXML = csgObject->getShapeXML();
  }
  file->putAttr("shape_xml", shapeXML);

  m_shape->material().saveNexus(file, "material");
  // Write out the other (indexes 1+) samples
  file->writeData("num_other_samples", int(m_samples.size()));
  for (size_t i = 0; i < m_samples.size(); i++)
    m_samples[i]->saveNexus(file, "sample" + Strings::toString(i + 1));
  // TODO: SampleEnvironment
  // OrientedLattice
  if (hasOrientedLattice()) {
    file->writeData("num_oriented_lattice", 1);
    m_lattice->saveNexus(file, "oriented_lattice");
  } else
    file->writeData("num_oriented_lattice", 0);

  // Legacy info from RAW file (I think)
  file->writeData("geom_id", m_geom_id);
  file->writeData("geom_thickness", m_thick);
  file->writeData("geom_height", m_height);
  file->writeData("geom_width", m_width);

  file->closeGroup();
}

//--------------------------------------------------------------------------------------------
/** Load the object from an open NeXus file.
 * @param file :: open NeXus file
 * @param group :: name of the group to open
 * @return the version tag of the sample group
 */
int Sample::loadNexus(::NeXus::File *file, const std::string &group) {
  file->openGroup(group, "NXsample");

  // Version 0 = saveNexusProcessed before Sep 8, 2011
  int version = 0;
  try {
    file->getAttr("version", version);
  } catch (::NeXus::Exception &) {
    version = 0;
  }

  if (version == 0) {
    // Sample NAME field may/may not be present
    try {
      file->readData("name", m_name);
    } catch (::NeXus::Exception &) {
      m_name = "";
    }
  }

  if (version > 0) {
    // Name is an attribute
    file->getAttr("name", m_name);
    if (file->hasAttr("name_empty")) {
      int isEmpty;
      file->getAttr("name_empty", isEmpty);
      if (isEmpty) {
        m_name.clear();
      }
    }

    // Shape (from XML)
    std::string shape_xml;
    file->getAttr("shape_xml", shape_xml);
    shape_xml = Strings::strip(shape_xml);
    if (!shape_xml.empty()) {
      ShapeFactory shapeMaker;
      m_shape = shapeMaker.createShape(shape_xml, false /*Don't wrap with <type> tag*/);
    }
    Kernel::Material material;
    material.loadNexus(file, "material");
    // CSGObject expected, if so, set its material
    if (auto csgObj = std::dynamic_pointer_cast<Geometry::CSGObject>(m_shape)) {
      csgObj->setMaterial(material);
    }

    // Load other samples
    int num_other_samples;
    file->readData("num_other_samples", num_other_samples);
    for (int i = 0; i < num_other_samples; i++) {
      auto extra = std::make_shared<Sample>();
      extra->loadNexus(file, "sample" + Strings::toString(i + 1));
      this->addSample(extra);
    }

    // OrientedLattice
    int num_oriented_lattice;
    file->readData("num_oriented_lattice", num_oriented_lattice);
    if (num_oriented_lattice > 0) {
      m_lattice = std::make_unique<OrientedLattice>();
      m_lattice->loadNexus(file, "oriented_lattice");
    }
  }

  try {
    // Legacy info from RAW file (I think)
    file->readData("geom_id", m_geom_id);
    file->readData("geom_thickness", m_thick);
    file->readData("geom_height", m_height);
    file->readData("geom_width", m_width);
  } catch (...) { /* Very old files don't have them. Ignore. */
  }

  file->closeGroup();

  return version;
}

/**
 * Delete the oriented lattice.
 */
void Sample::clearOrientedLattice() {
  if (m_lattice) {
    m_lattice.reset(nullptr);
  }
}

bool Sample::operator==(const Sample &other) const {
  if (m_samples.size() != other.m_samples.size())
    return false;
  for (size_t i = 0; i < m_samples.size(); ++i) {
    if (*m_samples[i] != *other.m_samples[i])
      return false;
  }
  auto compare = [](const auto &a, const auto &b, auto call_on) {
    // both null or both not null
    if (bool(a) ^ bool(b))
      return false;
    if (a)
      return call_on(a) == call_on(b);
    else
      return true;
  };
  return *m_lattice == *other.m_lattice && this->m_name == other.m_name && this->m_height == other.m_height &&
         this->m_width == other.m_width && this->m_thick == other.m_thick && m_geom_id == other.m_geom_id &&
         compare(m_environment, other.m_environment, [](const auto &x) { return x->name(); }) &&
         compare(m_shape, other.m_shape, [](const auto &x) { return x->shape(); }) &&
         compare(m_crystalStructure, other.m_crystalStructure, [](const auto &x) { return *(x->spaceGroup()); });
}

bool Sample::operator!=(const Sample &other) const { return !this->operator==(other); }
} // namespace Mantid::API

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Sample.h"
#include "MantidAPI/SampleEnvironment.h"
#include "MantidGeometry/IComponent.h"

namespace Mantid
{

  namespace API
  {

    using Geometry::Object;
    using Geometry::Material;
    using Geometry::V3D;
    
    /**
     * Default constructor
     */
    Sample::Sample() : 
      m_name(), m_shape(), m_material(), m_environment(),
      m_geom_id(0), m_thick(0.0), m_height(0.0), m_width(0.0)
    {
    }

    /** 
     * Copy constructor 
     *  @param copy :: const reference to the sample object
     */
    Sample::Sample(const Sample& copy) :
      m_name(copy.m_name), m_shape(copy.m_shape), m_material(copy.m_material), 
      m_environment(copy.m_environment), m_geom_id(copy.m_geom_id), m_thick(copy.m_thick), 
      m_height(copy.m_height), m_width(copy.m_width)
    {
    }

    /** Assignment operator 
     * @param rhs :: const reference to the sample object
     * @return A reference to this object, which will have the same 
     * state as the argument
     */
    Sample& Sample::operator=(const Sample&rhs)
    {
      if (this == &rhs) return *this;
      m_name = rhs.m_name;
      m_shape = rhs.m_shape;
      m_material = rhs.m_material;
      m_environment = rhs.m_environment;
      m_geom_id = rhs.m_geom_id;
      m_thick = rhs.m_thick;
      m_height = rhs.m_height;
      m_width = rhs.m_width;
      return *this;
    }
  
    /** 
     * Returns the name of the sample
     * @returns The name of this  sample
     */
    const std::string & Sample::getName() const
    {
      return m_name;
    }

    /** 
     * Update the name of the sample
     * @param name :: The name of the sample
     */
    void Sample::setName(const std::string & name)
    {
      m_name = name;
    }

    /**
     * Get a pointer to the sample shape object. It is assumed that this is defined within
     * its own coordinate system with its centre at [0,0,0]
     * @return A reference to the object describing the shape
     */
    const Object& Sample::getShape() const
    {
      return m_shape;
    }

    /** Set the object that describes the sample shape. It is assumed that this is defined such
     * that its centre is at [0,0,0]
     * @param object :: The object describing the shape
     * @throw An std::invalid_argument error if the object does 
     * not have a valid shape
     */
    void Sample::setShape(const Object & object)
    {
      if( object.hasValidShape() )
      {
        m_shape = object;
      }
      else
      {
        throw std::invalid_argument("Sample::setShape - Object has an invalid shape.");
      }
    }

    /** Return the material.
     * @return A reference to the material the sample is composed of
     */
    const Material & Sample::getMaterial() const
    {
      return m_material;
    }
    
    /**
     * Set the type of material that this sample is composed from
     * @param material :: A reference to the material object. It is copied into the sample.
     */
    void Sample::setMaterial(const Geometry::Material& material)
    {
      m_material = material;
    }

    /**
     * Return a reference to the sample environment that this sample is attached to
     * @return A const reference to a SampleEnvironment object
     * @throw std::runtime_error If the environment has not been defined
     */
    const SampleEnvironment & Sample::getEnvironment() const
    {
      if( !m_environment )
      {
        throw std::runtime_error("Sample::getEnvironment - No sample enviroment has been defined.");
      }
      return *m_environment;
    }

    /**
     * Attach an environment onto this sample
     * @param env :: A pointer to a created sample environment. This takes 
     * ownership of the object.
     */
    void Sample::setEnvironment(SampleEnvironment * env)
    {
      m_environment = boost::shared_ptr<SampleEnvironment>(env);
    }

    /** Return a const reference to the OrientedLattice of this sample
     * @return A const reference to a OrientedLattice object
     * @throw std::runtime_error If the OrientedLattice has not been defined
     */
    const OrientedLattice & Sample::getOrientedLattice() const
    {
      if( !m_lattice )
      {
        throw std::runtime_error("Sample::getOrientedLattice - No OrientedLattice has been defined.");
      }
      return *m_lattice;
    }

    /** Return a reference to the OrientedLattice of this sample
     * @return A reference to a OrientedLattice object
     * @throw std::runtime_error If the OrientedLattice has not been defined
     */
    OrientedLattice & Sample::getOrientedLattice()
    {
      if( !m_lattice )
      {
        throw std::runtime_error("Sample::getOrientedLattice - No OrientedLattice has been defined.");
      }
      return *m_lattice;
    }

    /** Attach an OrientedLattice onto this sample
     *
     * @param env :: A pointer to a OrientedLattice. This takes
     * ownership of the object.
     */
    void Sample::setOrientedLattice(OrientedLattice * latt)
    {
      m_lattice = boost::shared_ptr<OrientedLattice>(latt);
    }

    /**
     * Set the geometry flag that is specfied in the raw file within the SPB_STRUCT
     * 1 = cylinder, 2 = flat plate, 3 = disc, 4 = single crystal
     * @param geom_id :: The flag for the geometry
     */
    void Sample::setGeometryFlag(int geom_id)
    {
      m_geom_id = geom_id;
    }

    /**
     * Get the geometry flag that is specified in the raw file within the SPB_STRUCT
     * 1 = cylinder, 2 = flat plate, 3 = disc, 4 = single crystal
     * @returns The flag for the sample geometry
     */
    int Sample::getGeometryFlag() const
    {
      return m_geom_id;
    }

    /**
     * Set the thickness value
     * @param thick :: The parameter e_thick in the SPB_STRUCT
     */
    void Sample::setThickness(double thick)
    {
      m_thick = thick;
    }

    /**
     * Get the thickness value
     * @returns The parameter thickness parameter
     */
    double Sample::getThickness() const
    {
      return m_thick;
    }

    /**
     * Set the height value
     * @param height :: The parameter e_height in the SPB_STRUCT
     */
    void Sample::setHeight(double height)
    {
      m_height = height;
    }

    /**
     * Get the height value
     * @returns The parameter height parameter
     */
    double Sample::getHeight() const
    {
      return m_height;
    }

    /**
     * Set the width value
     * @param width :: The parameter e_width in the SPB_STRUCT
     */
    void Sample::setWidth(double width)
    {
      m_width = width;
    }

    /**
     * Get the height value
     * @returns The parameter height parameter
     */
    double Sample::getWidth() const
    {
      return m_width;
    }

  }
}

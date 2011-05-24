#ifndef MANTID_API_SAMPLE_H_
#define MANTID_API_SAMPLE_H_

//-----------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "DllExport.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Objects/Material.h"
#include <MantidGeometry/Crystal/OrientedLattice.h>

using Mantid::Geometry::OrientedLattice;

namespace Mantid
{

  namespace API
  {
    //-----------------------------------------------------------------------------
    // API forward declarations
    //------------------------------------------------------------------------------
    class SampleEnvironment;

    /** 

      This class stores information about the sample used in particular 
      run. It is a type of ObjComponent meaning it has a shape, a position
      and a material.

      @author Russell Taylor, Tessella plc
      @author Martyn Gigg, Tessella plc
      @date 26/11/2007
      
      Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
      
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
      
      File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
      Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class EXPORT_OPT_MANTID_API Sample
    {
    public:
      /// Default constructor (required for cow_ptr)
      Sample();
      /// Copy constructor. 
      Sample(const Sample& copy);
      /// Private assignment operator. 
      Sample& operator=(const Sample& rhs);

      /// Returns the name of the sample
      const std::string & getName() const;
      /// Set the name of the sample
      void setName(const std::string & name);
      /// Return the sample shape
      const Geometry::Object& getShape() const;
      /// Update the shape of the object
      void setShape(const Geometry::Object& shape);

      /** @name Material properties.*/
      //@{
      /// Return the material
      const Geometry::Material & getMaterial() const;
      /// Set the type of material that this sample is composed from
      void setMaterial(const Geometry::Material& material);
      //@}

      /** @name Access the environment information */
      //@{
      /// Get a reference to the sample's environment
      const SampleEnvironment & getEnvironment() const;
      /// Set the environment used to contain the sample
      void setEnvironment(SampleEnvironment * env);
      //@}

      /** @name Access the sample's lattice structure and orientation */
      //@{
      /// Get a reference to the sample's OrientedLattice
      const OrientedLattice & getOrientedLattice() const;
      /// Set the OrientedLattice defining the sample's lattice and orientation
      void setOrientedLattice(OrientedLattice * latt);
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

    private: 
      /// The sample name
      std::string m_name;
      /// The sample shape object
      Geometry::Object m_shape;
      /// The sample composition
      Geometry::Material m_material;
      /// An owned pointer to the SampleEnvironment object
      boost::shared_ptr<SampleEnvironment> m_environment;
      /// An owned pointer to the OrientedLattice object
      boost::shared_ptr<OrientedLattice> m_lattice;

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

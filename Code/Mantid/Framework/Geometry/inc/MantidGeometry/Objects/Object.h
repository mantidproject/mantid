#ifndef MANTID_GEOMETRY_OBJECT_H_
#define MANTID_GEOMETRY_OBJECT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"
#include "BoundingBox.h"
#include <map>

namespace Mantid
{
  //----------------------------------------------------------------------
  // Forward declarations
  //----------------------------------------------------------------------
  namespace Geometry
  {
    class Rule;
    class CompGrp;
    class Surface;
    class Track;
    class GeometryHandler;
    class CacheGeometryHandler;
    class vtkGeometryCacheReader;
    class vtkGeometryCacheWriter;

    /**
    \class Object
    \brief Global object for object
    \version 1.0
    \date July 2007
    \author S. Ansell

    An object is a collection of Rules and surface objects

    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class MANTID_GEOMETRY_DLL Object
    {
    public:
      /// Default constructor
      Object();
      /// Constructor providing shape xml.
      Object(const std::string& shapeXML);
      /// Copy constructor
      Object(const Object&);
      /// Assignment operator
      Object& operator=(const Object&);
      /// Destructor
      virtual ~Object();

      /// Return the top rule
      const Rule* topRule() const { return TopRule; }

      void setName(const int nx) { ObjName=nx; }           ///< Set Name
      int getName() const  { return ObjName; }             ///< Get Name

      void setMaterial(const Kernel::Material & material);
      const Kernel::Material & material() const;

      /// Return whether this object has a valid shape
      bool hasValidShape() const;
      int setObject(const int ON,const std::string& Ln);
      int procString(const std::string& Line);
      int complementaryObject(const int Cnum,std::string& Ln); ///< Process a complementary object
      int hasComplement() const;

      int populate(const std::map<int,Surface*>&);
      int createSurfaceList(const int outFlag=0);               ///< create Surface list
      int addSurfString(const std::string&);     ///< Not implemented
      int removeSurface(const int SurfN);
      int substituteSurf(const int SurfN,const int NsurfN,Surface* SPtr);
      void makeComplement();
      void convertComplement(const std::map<int,Object>&);

      virtual void print() const;
      void printTree() const;

      bool isValid(const Kernel::V3D&) const;    ///< Check if a point is valid
      bool isValid(const std::map<int,int>&) const;  ///< Check if a set of surfaces are valid.
      bool isOnSide(const Kernel::V3D&) const;
      int calcValidType(const Kernel::V3D& Pt,const Kernel::V3D& uVec) const;

      std::vector<int> getSurfaceIndex() const;
      /// Get the list of surfaces (const version)
      const std::vector<const Surface*>& getSurfacePtr() const { return SurList; }
      /// Get the list of surfaces
      std::vector<const Surface*>& getSurfacePtr() { return SurList; }

      std::string cellCompStr() const;
      std::string cellStr(const std::map<int,Object>&) const;

      std::string str() const;
      void write(std::ostream&) const;     ///< MCNPX output

      // INTERSECTION
      int interceptSurface(Geometry::Track&) const;

      // Solid angle - uses triangleSolidAngle unless many (>30000) triangles
      double solidAngle(const Kernel::V3D& observer) const;
      // Solid angle with a scaling of the object
      double solidAngle(const Kernel::V3D& observer, const Kernel::V3D& scaleFactor) const;
      // solid angle via triangulation
      double triangleSolidAngle(const Kernel::V3D& observer) const;
      // Solid angle via triangulation with scaling factor for object size
      double triangleSolidAngle(const Kernel::V3D& observer, const Kernel::V3D& scaleFactor) const;
      // solid angle via ray tracing
      double rayTraceSolidAngle(const Kernel::V3D& observer) const;

      /// Calculate (or return cached value of) Axis Aligned Bounding box (DEPRECATED)
      void getBoundingBox(double& xmax,double& ymax,double& zmax,double& xmin,double& ymin,double& zmin) const;

      /// Return cached value of axis-aligned bounding box
      const BoundingBox & getBoundingBox() const;
      /// Define axis-aligned bounding box
      void defineBoundingBox(const double& xmax,const double& ymax,const double& zmax,const double& xmin,const double& ymin,const double& zmin);
      /// Set a null bounding box for this object
      void setNullBoundingBox();
      // find internal point to object
      int getPointInObject(Kernel::V3D& point) const;

      //Rendering member functions
      void draw() const;
      //Initialize Drawing
      void initDraw() const;
      //Get Geometry Handler
      boost::shared_ptr<GeometryHandler> getGeometryHandler();
      /// Set Geometry Handler
      void setGeometryHandler(boost::shared_ptr<GeometryHandler> h);

      ///set vtkGeometryCache writer
      void setVtkGeometryCacheWriter(boost::shared_ptr<vtkGeometryCacheWriter>);
      ///set vtkGeometryCache reader
      void setVtkGeometryCacheReader(boost::shared_ptr<vtkGeometryCacheReader>);
      void GetObjectGeom(int& type, std::vector<Kernel::V3D>& vectors, double& myradius, double & myheight) const;
      /// Getter for the shape xml
      std::string getShapeXML() const;
    private:
      int ObjName;       ///< Creation number
      Rule* TopRule;     ///< Top rule [ Geometric scope of object]

      int procPair(std::string& Ln,std::map<int,Rule*>& Rlist,int& compUnit) const;
      CompGrp* procComp(Rule*) const;
      int checkSurfaceValid(const Kernel::V3D&,const Kernel::V3D&) const;
      BoundingBox m_boundingBox; ///< Object's bounding box

      // -- DEPRECATED --
      mutable double AABBxMax,  ///< xmax of Axis aligned bounding box cache
        AABByMax,  ///< ymax of Axis aligned bounding box cache
        AABBzMax,  ///< zmax of Axis aligned bounding box cache
        AABBxMin,  ///< xmin of Axis aligned bounding box cache
        AABByMin,  ///< xmin of Axis aligned bounding box cache
        AABBzMin;  ///< zmin of Axis Aligned Bounding Box Cache
      mutable bool  boolBounded; ///< flag true if a bounding box exists, either by getBoundingBox or defineBoundingBox
      // -- --

      int searchForObject(Kernel::V3D&) const;
      double getTriangleSolidAngle(const Kernel::V3D& a, const Kernel::V3D& b, const Kernel::V3D& c, const Kernel::V3D& observer) const;
      double CuboidSolidAngle(const Kernel::V3D observer, const std::vector<Kernel::V3D> vectors) const;
      double SphereSolidAngle(const Kernel::V3D observer, const std::vector<Kernel::V3D> vectors, const double radius) const;
      double CylinderSolidAngle(const Kernel::V3D & observer, const Mantid::Kernel::V3D & centre,
        const Mantid::Kernel::V3D & axis,
        const double radius, const double height) const;
      double ConeSolidAngle(const Kernel::V3D & observer, const Mantid::Kernel::V3D & centre,
        const Mantid::Kernel::V3D & axis,
        const double radius, const double height) const;

      /// Geometry Handle for rendering
      boost::shared_ptr<GeometryHandler> handle;
      friend class CacheGeometryHandler;
      /// Is geometry caching enabled?
      bool bGeometryCaching;
      /// a pointer to a class for reading from the geometry cache
      boost::shared_ptr<vtkGeometryCacheReader> vtkCacheReader;
      /// a pointer to a class for writing to the geometry cache
      boost::shared_ptr<vtkGeometryCacheWriter> vtkCacheWriter;
      void updateGeometryHandler();
      /// for solid angle from triangulation
      int NumberOfTriangles() const;
      int NumberOfPoints() const;
      int* getTriangleFaces() const;
      double* getTriangleVertices() const;
      /// original shape xml used to generate this object.
      std::string m_shapeXML;
      /// material composition
      Kernel::Material m_material;

    protected:
      std::vector<const Surface*> SurList;  ///< Full surfaces (make a map including complementary object ?)
    };

    /// Typdef for a shared pointer
    typedef boost::shared_ptr<Object> Object_sptr;
    /// Typdef for a shared pointer to a const object
    typedef boost::shared_ptr<const Object> Object_const_sptr;

  }  // NAMESPACE Geometry
}  // NAMESPACE Mantid

#endif /*MANTID_GEOMETRY_OBJECT_H_*/

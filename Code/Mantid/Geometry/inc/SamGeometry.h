#ifndef SamGeometry_h
#define SamGeometry_h


namespace Mantid
{

namespace Geometry
{
  /*!
    \class SamGeometry
    \version 1.0
    \author S. Ansel
    \date July 2007
    \brief Holds the sample description
    
    The class hold Object directly.
    Surfaces and Materials [both deep inherrited objects]
    are held in the main simulation system and this
    class has a pointer to that memory.
  */

class SamGeometry
{

 public:

  typedef std::vector<Object> ISTORE;             ///< Storage for Objects
  typedef std::map<int,const Surface*> SMAP;      ///< Map of surfaces
  typedef std::multimap<int,int> MOBJ;            ///< Connecting Objects
  typedef std::map<int,const Material*> MATMAP;   ///< Map of Materials.

 private:

  static Logger& PLog;           ///< The official logger

  ISTORE Items;                     ///< Items;
  SMAP SNum;                        ///< SNum : Surface
  MOBJ SurToObj;                    ///< SNum : ObjectNum
  MATMAP MatMap;                    ///< Int : Materials [Un managed pointers]

  // Optimization stuff here:
  void createTable();      ///< Create the table
  /// Build a track
  Geometry::Track buildTrack(const Geometry::Vec3D&,
			     const Geometry::Vec3D&) const;

 public:

  SamGeometry();                               ///< Constructor
  SamGeometry(const SamGeometry&);             ///< Copy constructor
  SamGeometry& operator=(const SamGeometry&);  ///< Copy assignment operator
  ~SamGeometry();                              ///< Destructor

  void addObject(const Object&);                ///< Add an object
  void setMaterial(const int,const Material*);  ///< Set the material

  ISTORE::iterator begin() { return Items.begin(); }  ///< Iterator pointing to the first item.
  ISTORE::iterator end() { return Items.end(); }      ///< Iterator pointing one-past-the-end

  const Object& getObject(const int) const;     ///< Get an object by index (const version)
  Object& getObject(const int);                 ///< Get an object by index

  int findCell(const Geometry::Vec3D&) const;            ///< Find a cell
  int findCell(const Geometry::Vec3D&,const int) const;  ///< Find a cell

  /// Transport stuff
  double outAtten(const double,const int,const Geometry::Vec3D&,
		  const Geometry::Vec3D&) const;


  void procXML(XML::XMLcollect&) const;      ///< Writes the XML schema
  int importXML(IndexIterator<XML::XMLobject,XML::XMLgroup>&,const int);   ///< Read in XML

};

}  // NAMESPACE MonteCarlo

}  // NAMESPACE Mantid

#endif
  

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
  void createTable();
  Geometry::Track buildTrack(const Geometry::Vec3D&,
			     const Geometry::Vec3D&) const;

 public:

  SamGeometry();
  SamGeometry(const SamGeometry&);
  SamGeometry& operator=(const SamGeometry&);
  ~SamGeometry();

  void addObject(const Object&);
  void setMaterial(const int,const Material*);

  ISTORE::iterator begin() { return Items.begin(); }
  ISTORE::iterator end() { return Items.end(); }

  const Object& getObject(const int) const;
  Object& getObject(const int);

  int findCell(const Geometry::Vec3D&) const;  
  int findCell(const Geometry::Vec3D&,const int) const;

  // Transport stuff
  double outAtten(const double,const int,const Geometry::Vec3D&,
		  const Geometry::Vec3D&) const;


  void procXML(XML::XMLcollect&) const;
  int importXML(IndexIterator<XML::XMLobject,XML::XMLgroup>&,const int);

};

}  // NAMESPACE MonteCarlo

}  // NAMESPACE Mantid

#endif
  

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

  Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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

  File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>

*/

class DLLExport SamGeometry
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


};

}  // NAMESPACE MonteCarlo

}  // NAMESPACE Mantid

#endif
  

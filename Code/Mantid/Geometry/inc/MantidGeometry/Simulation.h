#ifndef Simulation_h
#define Simulation_h

// RJT, 3/12/07: Apologies for the comments in here. They're simply in there to stop doxygen complaining
// and I didn't have anything to go on apart from the method names.

namespace Mantid
{

namespace Geometry
{

/*!
  \class Simulation
  \version 1.5
  \date July 2007
  \author S.Ansell
  \brief Containes all the information on the simulations.

  Contains the running simulation information 
  Mainly list of maps and process information
  Is expected to be mainly a singleton class.

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

class DLLExport Simulation
{
 private:

  static Logger& PLog;           ///< The official logger
  
  std::map<int,Material*> MList;         ///< Material List (key = MatNumber)
  std::map<int,Surface*> SurMap;         ///< Surface Map (key = Surface Num)
  
  Beam B;                                ///< Main beam
   
  SamGeometry Vanadium;                  ///< Vanadium
  SamGeometry Container;                 ///< Container
  SamGeometry Sample;                    ///< Sample

  Detector VDetector;                    ///< Vanadium detector
  Detector CDetector;                    ///< Container detector
  Detector SDetector;                    ///< Sample detector

  SamGeometry* cSam;                      ///< current detector value  
  Detector* cDet;                     ///< current detector value  

  int populateCells();                 ///< Set the cells 
  int populateDetectors();               ///< Set the detectors
  int setMaterialDensity();               ///< Set the material density
  int checkInsert(const Object&);            ///< Inserts (and test) new hull into Olist map 
  int removeComplements();                        ///< Remove the complements!
  int removeComplement(Object&) const;            ///< Remove a complement!
  int replaceSurface(const int,Surface*);         ///< Replace a surface
  int getMaterialNum(const std::string&) const;   ///< Get the material number of the argument provided

  template<int Index>  SamGeometry& getSamGeom();  ///< Get Sample Geometry

 public:

  Simulation();                                ///< Constructor
  Simulation(const Simulation&);               ///< Copy constructor
  Simulation& operator=(const Simulation&);    ///< Copy assignment operator
  ~Simulation();                               ///< Destructor

  void readMaster(const std::string&);         ///< Read the master!
  void populateWCells();                       ///< Populate cells
  int isValidCell(const int,const Geometry::Vec3D&) const;   ///< Is this a valid cell?

  Object* findObject(const int);              ///< Determine a hull for a cell
  const Object* findObject(const int) const;  ///< return a hull (if processed)
  int findCell(const Geometry::Vec3D&,const int=0) const;  ///< Find a cell
  int existCell(const int) const;              ///< check if cell exist
  int addCell(const int,const Object&);         ///< Adds a new hull into Olist map 
  int bindCell(const int,const int);            ///< Bind a cell
  std::vector<int> getCellVector() const;       ///< Get a cell vector
  std::vector<int> getCellWithMaterial(const int) const;   ///< Get a cell

  int addMaterial(const std::string&,const Material&); ///< Add a material
  int addMaterial(const int,const Material&);          ///< Add a material
  

  // BEAM
  Beam& getBeam() { return B; }    ///< Get a reference to the beam

  // SURFACE
  int findSurfID(const int) const;             ///< returns surface Id

  // Material
  int getCellMaterial(const int) const;        ///< return cell material
  const Material* getMaterial(const int) const;    ///< Get the material with given index
  

  void refineSim();                            ///< Refine the simulation!
  void write(const std::string&) const;        ///< Printout method
  void writeCinder() const;                    ///< Print method

  // LOAD/BUILD
  int createSurface(const int,const std::string&);                            ///< Create a surface
  template<int SGindex>  int createObject(const int,const std::string&);      ///< Create an object
  template<int SGindex> void setObjectMaterial(const int,const int);          ///< Set an object's material
  template<int SGindex> void setObjectMaterial(const int,const std::string&); ///< Set an object's material



  // Direct Monte Carlo
  
  void addSingle(const Geometry::Vec3D&,const double,const int);   ///< Add a single something
  void addMult(const Geometry::Vec3D&,const double,const int);     ///< Add multiple somethings

  // Debug stuff
  
  void printCell(const int) const;                ///< Print out the cell
  void printVertex(const int) const;              ///< Print out the vertex
  int checkSurface(const int,const Geometry::Vec3D&) const;    ///< Check a surface
  int checkSurfGen(const int,const Geometry::Vec3D&) const;    ///< Check a surface
  void printGeneral(const int) const;             ///< Print out method
  void printSurface(const int) const;             ///< Print out method

};

}  // NAMESPACE MonteCarlo

}  // NAMESPACE Mantid

#endif

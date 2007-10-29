#ifndef Simulation_h
#define Simulation_h

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
*/

class Simulation
{
 private:
  
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

  int populateCells();                  
  int populateDetectors();               ///< Set the detectors
  int setMaterialDensity();
  int checkInsert(const Object&);            ///< Inserts (and test) new hull into Olist map 
  int removeComplements();
  int removeComplement(Object&) const;
  int replaceSurface(const int,Surface*);
  int getMaterialNum(const std::string&) const;

  template<int Index>  SamGeometry& getSamGeom();

 public:

  Simulation();
  Simulation(const Simulation&);
  Simulation& operator=(const Simulation&);
  ~Simulation();

  void readMaster(const std::string&);   
  void populateWCells();
  int isValidCell(const int,const Geometry::Vec3D&) const;

  Object* findObject(const int);              ///< Determine a hull for a cell
  const Object* findObject(const int) const;  ///< return a hull (if processed)
  int findCell(const Geometry::Vec3D&,const int=0) const;  
  int existCell(const int) const;              ///< check if cell exist
  int addCell(const int,const Object&);         ///< Adds a new hull into Olist map 
  int bindCell(const int,const int);
  std::vector<int> getCellVector() const;
  std::vector<int> getCellWithMaterial(const int) const;

  int addMaterial(const std::string&,const Material&);
  int addMaterial(const int,const Material&);
  

  // BEAM
  Beam& getBeam() { return B; }

  // SURFACE
  int findSurfID(const int) const;             ///< returns surface Id

  // Material
  int getCellMaterial(const int) const;        ///< return cell material
  const Material* getMaterial(const int) const; 
  

  void refineSim();
  void write(const std::string&) const;  
  void writeCinder() const;          

  // LOAD/BUILD
  int createSurface(const int,const std::string&);
  template<int SGindex>  int createObject(const int,const std::string&);
  template<int SGindex> void setObjectMaterial(const int,const int);
  template<int SGindex> void setObjectMaterial(const int,const std::string&);

  // XML 
  void procXML(XML::XMLcollect&) const;
  int importXML(IndexIterator<XML::XMLobject,XML::XMLgroup>&,const int);
  void writeXML(const std::string&) const;
  int loadXML(const std::string&);

  // Direct Monte Carlo
  
  void addSingle(const Geometry::Vec3D&,const double,const int);
  void addMult(const Geometry::Vec3D&,const double,const int);  

  // Debug stuff
  
  void printCell(const int) const;               
  void printVertex(const int) const;
  int checkSurface(const int,const Geometry::Vec3D&) const;
  int checkSurfGen(const int,const Geometry::Vec3D&) const;
  void printGeneral(const int) const;
  void printSurface(const int) const;

};

}  // NAMESPACE MonteCarlo

#endif

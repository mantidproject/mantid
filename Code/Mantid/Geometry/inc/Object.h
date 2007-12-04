#ifndef Object_h
#define Object_h

namespace Mantid
{

namespace Geometry
{

/*!
  \class Object
  \brief Global object for object 
  \version 1.0
  \date July 2007
  \author S. Ansell

  An object is a collection of Rules and
  surface object
*/

class Object
{
 private:

  static Kernel::Logger& PLog;           ///< The official logger
  
  int ObjName;       ///< Creation number
  int MatN;          ///< Material Number   ???
  double Tmp;        ///< Temperature (K)   
  double density;    ///< Density           

  Rule* TopRule;     ///< Top rule [ Geometric scope of object]
  
  int procPair(std::string& Ln,std::map<int,Rule*>& Rlist,int& compUnit) const;
  CompGrp* procComp(Rule*) const;
  int checkSurfaceValid(const Geometry::Vec3D&,const Geometry::Vec3D&) const;

 protected:
  
  std::vector<const Surface*> SurList;  ///< Full surfaces (make a map including complementary object ?)

  /// Return the top rule
  const Rule* topRule() const { return TopRule; }
  
 public:
  
  Object();
  Object(const Object&);
  Object& operator=(const Object&);
  virtual ~Object();

  void setName(const int nx) { ObjName=nx; }           ///< Set Name 
  void setTemp(const double A) { Tmp=A; }              ///< Set temperature [Kelvin]
  int setObject(const int ON,const std::string& Ln);        
  int procString(const std::string& Line);                 
  void setDensity(const double D) { density=D; }       ///< Set Density [Atom/A^3]

  int complementaryObject(const int Cnum,std::string& Ln);     ///< Process a complementary object
  int hasComplement() const;                         
  
  int getName() const  { return ObjName; }             ///< Get Name
  int getMat() const { return MatN; }                  ///< Get Material ID
  double getTemp() const { return Tmp; }               ///< Get Temperature [K]
  double getDensity() const { return density; }        ///< Get Density [Atom/A^3]

  int populate(const std::map<int,Surface*>&);
  int createSurfaceList(const int outFlag=0);               ///< create Surface list
  int addSurfString(const std::string&);     ///< Not implemented
  int removeSurface(const int SurfN);                    
  int substituteSurf(const int SurfN,const int NsurfN,Surface* SPtr);  
  void makeComplement();
  void convertComplement(const std::map<int,Object>&);

  virtual void print() const;
  void printTree() const;
  
  int isValid(const Geometry::Vec3D&) const;    ///< Check if a point is valid
  int isValid(const std::map<int,int>&) const;  ///< Check if a set of surfaces are valid.
  int isOnSide(const Geometry::Vec3D&) const;
  int calcValidType(const Geometry::Vec3D& Pt,const Geometry::Vec3D& uVec) const;

  std::vector<int> getSurfaceIndex() const;
  /// Get the list of surfaces (const version)
  const std::vector<const Surface*>& getSurfacePtr() const 
    { return SurList; } 
  /// Get the list of surfaces
  std::vector<const Surface*>& getSurfacePtr()
    { return SurList; } 

  std::string cellCompStr() const;
  std::string cellStr(const std::map<int,Object>&) const;

  std::string str() const;
  void write(std::ostream&) const;     ///< MCNPX output

  // INTERSECTION
  int interceptSurface(Geometry::Track&) const;

  // XML:
  void procXML(XML::XMLcollect&) const;
  int importXML(IndexIterator<XML::XMLobject,XML::XMLgroup>& SK,
		const int singleFlag=0);

};

}   // NAMESPACE MonteCarlo
}  // NAMESPACE Mantid

#endif

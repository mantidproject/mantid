#ifndef surfaceFactory_h
#define surfaceFactory_h


/*!
  \namespace MonteCarlo
  \brief Holds the  MonteCarlo neutron correction code
  \author S. Ansell
  \version 1.0
  \date July 2007
  
*/

namespace Mantid
{

namespace Geometry
{

/*!
  \class surfaceFactory
  \brief creates instances of tallies
  \version 1.0
  \date May 2006
  \author S. Ansell
  
  This is a singleton class.
  It creates Surface* depending registered tallies
  and the key given. Predominately for creating
  tallies from an input deck where we only have the number.
*/

class surfaceFactory
{
 private:

  static Kernel::Logger& PLog;           ///< The official logger

  typedef std::map<std::string,Surface*> MapType;     ///< Storage of surface pointers
  
  static surfaceFactory* FOBJ;             ///< Effective "this"
 
  MapType SGrid;                           ///< The tally stack
  std::map<char,std::string> ID;           ///< Short letter identifiers

  surfaceFactory();                        ///< singleton constructor
  surfaceFactory(const surfaceFactory&);

  /// Dummy assignment operator
  surfaceFactory& operator=(const surfaceFactory&)   
    { return *this; } 

  void registerSurface();

 public:

  static surfaceFactory* Instance();
  ~surfaceFactory();
  
  Surface* createSurface(const std::string&) const;
  Surface* createSurfaceID(const std::string&) const;

};

} // NAMESPACE Geometry

}  // NAMESPACE Mantid

#endif


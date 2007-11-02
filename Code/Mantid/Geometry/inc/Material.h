#ifndef Material_h
#define Material_h

namespace Mantid
{

namespace Geometry
{
  /*!
    \class Material
    \brief Nuetronic information on the material
    \author S. Ansell
    \version 1.0
    \date July 2007
    
    This can be extended so that more sophisticated material
    components can be used.
    \todo This class nees to have a base class.
  */
  
class Material
{
 private:

  static Logger& PLog;           ///< The official logger
  
  std::string Name;      ///< Name 
  double density;        ///< number density [atom/A^3]
  double scoh;           ///< scattering cross section 
  double sinc;           ///< incoherrrent cross section 
  double sabs;           ///< Absorption cross section
  
 public:
  
  Material();
  Material(const std::string&,const double,
	   const double,const double,const double);
  Material(const double,const double,const double,const double);
  Material(const Material&);
  virtual Material* clone() const;
  Material& operator=(const Material&);
  virtual ~Material();
  
  void setName(const std::string& N) { Name=N; }
  void setDensity(const double);
  void setScat(const double,const double,const double);


  double getAtomDensity() const { return density; }

  const std::string& getName() const { return Name; }
  double getScat() const { return scoh+sinc; }
  double getCoh() const { return scoh; }
  double getInc() const { return sinc; }
  double getScatFrac(const double) const;
  double getAtten(const double) const; 
  double getAttenAbs(const double) const; 

  double calcAtten(const double,const double) const;
  
  virtual void procXML(XML::XMLcollect&) const;
  virtual int importXML(IndexIterator<XML::XMLobject,XML::XMLgroup>&,
		const int =0);

};


} // NAMESPACE MonteCarlo

}  // NAMESPACE Mantid

#endif

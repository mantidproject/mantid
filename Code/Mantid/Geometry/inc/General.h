#ifndef General_h
#define General_h

namespace MonteCarlo
{

/*!
  \class General
  \brief Holds a general quadratic surface
  \author S. Ansell
  \date April 2004
  \version 1.0

  Holds a general surface with equation form
  \f[ Ax^2+By^2+Cz^2+Dxy+Exz+Fyz+Gx+Hy+Jz+K=0 \f]
  which has been defined as a gq surface in MCNPX.
  It is a realisation of the Surface object.
*/


class General : public Surface
{
 private:
  
 public:
    
  General();
  General(const General&);
  General* clone() const;
  General& operator=(const General&);
  ~General();
  
  int setSurface(const std::string&);

  void setBaseEqn();

  void procXML(XML::XMLcollect&) const;
  int importXML(IndexIterator<XML::XMLobject,XML::XMLgroup>&,const int=0);

};

}  // NAMESPACE MonteCarlo

#endif

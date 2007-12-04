#ifndef Algebra_h
#define Algebra_h


namespace Mantid
{

namespace Geometry
{

/*!
  \class  Algebra
  \brief Computes Boolean algebra for simplification
  \author S. Ansell
  \date August 2005
  \version 1.0

  A leveled algebra class for each
  level of factorisation. 
*/ 

class Algebra
{
 private:

  static Kernel::Logger& PLog;           ///< The official logger

  std::map<int,std::string> SurfMap;     ///< Internal surface map
  Acomp F;       ///< Factor
  
 public:

  Algebra();
  Algebra(const Algebra&);
  Algebra& operator=(const Algebra&);
  ~Algebra();
  
  int operator==(const Algebra&) const;
  Algebra& operator+=(const Algebra&);
  Algebra& operator*=(const Algebra&);
  Algebra operator+(const Algebra&) const;
  Algebra operator*(const Algebra&) const;


  void Complement();
  void makeDNF() { F.makeDNFobject(); }  ///< assessor to makeDNFobj
  void makeCNF() { F.makeCNFobject(); }  ///< assessor to makeCNFobj
  std::pair<Algebra,Algebra> algDiv(const Algebra&) const;
  int setFunctionObjStr(const std::string&);
  int setFunction(const std::string&);
  int setFunction(const Acomp&);

  std::ostream& write(std::ostream&) const;
  std::string writeMCNPX() const;

  // Debug Functions::
  int countLiterals() const;

};

}  // NAMESPACE Geometry

}  // NAMESPACE Mantid

#endif 

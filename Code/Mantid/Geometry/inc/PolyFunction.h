#ifndef mathLevel_PolyFunction_h
#define mathLevel_PolyFunction_h

namespace Mantid
{

namespace mathLevel
{

  /*!
    \class PolyFunction
    \version 1.0
    \author S. Ansell 
    \date December 2007
    \brief Holds a polynominal base type
  */

class PolyFunction
{
 protected:

  double Eaccuracy;               ///< Polynomic accuracy

 public:

  PolyFunction();
  explicit PolyFunction(double const);
  PolyFunction(const PolyFunction&);
  PolyFunction& operator=(const PolyFunction&);
  virtual ~PolyFunction();

  virtual PolyFunction& operator+=(double const) =0;  ///< Virtual add
  virtual PolyFunction& operator/=(double const) =0;  ///< Virtual divide

  void write(std::ostream&) const;
};

std::ostream& operator<<(std::ostream&,const PolyFunction&);


}  // NAMESPACE mathlevel

} // NAMESPACE Mantid

#endif

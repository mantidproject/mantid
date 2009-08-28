#ifndef mathLevel_PolyFunction_h
#define mathLevel_PolyFunction_h

#include "MantidKernel/System.h"

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


class DLLExport PolyFunction
{
 protected:

  double Eaccuracy;               ///< Polynomic accuracy

 public:

  static int getMaxSize(const std::string& CLine,const char V);

  PolyFunction();
  explicit PolyFunction(double const);
  PolyFunction(const PolyFunction&);
  PolyFunction& operator=(const PolyFunction&);
  virtual ~PolyFunction();

  /// Abstract Self Addition opterator for a double
  virtual PolyFunction& operator+=(const double) =0;
  /// Abstract Self Division opterator for a double
  virtual PolyFunction& operator/=(const double) =0;

  void write(std::ostream&) const;
};

std::ostream& operator<<(std::ostream&,const PolyFunction&);


}  // NAMESPACE mathlevel


} // NAMESPACE Mantid

#endif

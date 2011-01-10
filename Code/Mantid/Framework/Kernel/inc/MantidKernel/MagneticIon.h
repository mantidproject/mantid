#ifndef MAGNETICION_H_
#define MAGNETICION_H_

#include <string>
#include <vector>
#include <map>
#include "MantidKernel/DllExport.h"
#include "MantidKernel/PhysicalConstants.h"

namespace Mantid
{
namespace PhysicalConstants
{
  /**
   * Struture to hold information about magnetic form factor for 3d, 4d, 
   * rare earth, and actinide atoms and ions. Data is taken from International Tables of 
   * Crystalography, volume C, section 4.4.5 http://it.iucr.org/Cb/ch4o4v0001/sec4o4o5/
   */
  struct EXPORT_OPT_MANTID_KERNEL MagneticIon {
    MagneticIon();
    MagneticIon(const std::string symbol, const uint16_t charge,const  double j0[8],
         const double j2[8],const  double j4[8],const  double j6[8]);
    MagneticIon(const MagneticIon& other);

    /// The atomic symbol. In other words the one or two character abbreviation.
    std::string symbol;

    /// The charge of the ion, or 0 for neutral atom. Note thet all charges are not negative
    uint16_t charge;
    
    /// A vector containing A, a, B, b, C, c D, e for each <j0>, <j2>, <j4>, and <j6> 
    std::vector <double> j0;
    std::vector <double> j2;
    std::vector <double> j4;
    std::vector <double> j6;
  };

  static std::map<std::string,MagneticIon> ion_map;
  int initializeMap();

  DLLExport MagneticIon getMagneticIon(const std::string symbol,const uint16_t charge);
  DLLExport std::vector <double> getJL(const std::string symbol,const uint16_t charge, const uint16_t l = 0);

} // namespace PhysicalConstants
} // namespace Mantid

#endif /* MAGNETICION_H_ */

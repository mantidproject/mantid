#ifndef RefCon_h
#define RefCon_h

/*!
  \namespace RefCon
  \brief Physics constants for Reflectivity
  \author S. Ansell
  \version 1.0
  \date September 2005
*/

namespace RefCon
{
  const double c2(8.987551787e16);            ///< Speed of light^2 [m^2sec^4]
  const double e2(2.567010233e-38);           ///< electron charge^2  
  const double me(9.109534e-31);              ///< Mass of electron (kg)
  const double hc_e(12.39852066);             ///< convertion in keV to angstrom (wavelength)
  const double reyb(13.60569172);             ///< Reyberg Constant
  const double pi(3.14159265359);             ///< Pi
  const double re(2.817940285e-15);           ///< electron Radius
  const double avogadro(0.602214);            ///< Avogadro in unis of gram/anstrom
};

#endif

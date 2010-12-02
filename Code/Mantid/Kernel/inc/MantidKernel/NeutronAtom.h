#ifndef MANTID_KERNEL_NEUTRONATOM_H_
#define MANTID_KERNEL_NEUTRONATOM_H_

//------------------------------------------------------------------------------
// Include
//------------------------------------------------------------------------------
#include <string>
#include "MantidKernel/DllExport.h"

namespace Mantid
{
  namespace PhysicalConstants
  {

    /** 
      Structure to store neutronic scattering information for the various elements.
      This is taken from http://www.ncnr.nist.gov/resources/n-lengths/list.html.
     
      Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
      
      This file is part of Mantid.
      
      Mantid is free software; you can redistribute it and/or modify
      it under the terms of the GNU General Public License as published by
      the Free Software Foundation; either version 3 of the License, or
      (at your option) any later version.
      
      Mantid is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
      GNU General Public License for more details.
      
      You should have received a copy of the GNU General Public License
      along with this program.  If not, see <http://www.gnu.org/licenses/>.
      
      File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.   
      Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    struct EXPORT_OPT_MANTID_KERNEL NeutronAtom {

      /// The reference wavelength value for absorption cross sections
      static const double ReferenceLambda;

      NeutronAtom(const uint8_t z,
		  const double coh_b_real, const double inc_b_real,
		  const double coh_xs, const double inc_xs,
		  const double tot_xs, const double abs_xs);

      NeutronAtom(const uint8_t z, const uint8_t a,
		  const double coh_b_real, const double inc_b_real,
		  const double coh_xs, const double inc_xs,
		  const double tot_xs, const double abs_xs);

      NeutronAtom(const uint8_t z, const uint8_t a,
		  const double coh_b_real, const double coh_b_img,
		  const double inc_b_real, const double inc_b_img,
		  const double coh_xs, const double inc_xs,
		  const double tot_xs, const double abs_xs);

      /// The atomic number, or number of protons, for the atom.
      uint8_t z_number;

      /// The total number of protons and neutrons, or mass number, 
      /// for the atom for isotopic averages this is set to zero.
      uint8_t a_number;

      /// The real part of the coherent scattering length in fm.
      double coh_scatt_length_real;

      /// The imaginary part of the coherent scattering length in fm.
      double coh_scatt_length_img;

      /// The real part of the incoherent scattering length in fm.
      double inc_scatt_length_real;

      /// The imaginary part of the incoherent scattering length in fm.
      double inc_scatt_length_img;

      /// The coherent scattering cross section in barns.
      double coh_scatt_xs;

      /// The incoherent scattering cross section in barns.
      double inc_scatt_xs;

      /// The total scattering cross section in barns.
      double tot_scatt_xs;

      /// The absorption cross section for 2200m/s neutrons in barns.
      double abs_scatt_xs;
    };

    DLLExport NeutronAtom getNeutronAtom(const int z_number, const int a_number = 0);

  } //Namespace PhysicalConstants
} //Namespace Mantid

#endif /* NEUTRONATOM_H_ */

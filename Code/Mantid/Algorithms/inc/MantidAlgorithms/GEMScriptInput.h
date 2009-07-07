#ifndef MANTID_ALGORITHMS_GEMSCRIPTINPUT_H_
#define MANTID_ALGORITHMS_GEMSCRIPTINPUT_H_

//-----------------------------------
// Includes
//-----------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{

/**
   Required properties:
   <UL>
   <LI>SampleFile - The sample GEM data file </LI>
  <LI>SampleBackgroundFile - The sample background GEM data file </LI>
  <LI>VanadiumFile - The vanadium GEM data file </LI>
  <LI>VanadiumBackgroundFile - The vanadium background GEM data file </LI>
  <LI>WorkingDir - The working directory for NeXuS files and output </LI>
  <LI>CalibFile - The calibration file </LI>

  <LI>SampleAbsTF - Sample absorption tickbox </LI>
  <LI>SampleBackgroundFocusTF - Sample background focusing tickbox </LI>
  <LI>VanadiumFocusTF - Vanadium focusing tickbox </LI>

   </UL>
   
    Optional properties:
   <UL>
  <LI>SampleRadius - The radius of the sample (cm)</LI>
  <LI>SampleHeight - The height of the sample (cm)</LI>
  <LI>SampleDensity - The density of the sample (atoms per A cubed)</LI>
  <LI>SampleSCS - The sample total scattering cross-section (barns)</LI>
  <LI>SampleACS - The sample aborption cross-section (barns)</LI>
  <LI>VanRadius - The radius of the V rod (cm)</LI>
  <LI>VanHeight - The height of the V rod (cm)</LI>
  <LI>VanDensity - The density of the V rod (atoms per A cubed)</LI>
  <LI>VanSCS - The V total scattering cross-section (barns)</LI>
  <LI>VanACS - The V aborption cross-section (barns)</LI>
   </UL>
   @author David Keen and Anders Markvardsen, ISIS Facility
   @date 26/02/2009
     
   Copyright &copy; 2009 STFC Rutherford Appleton Laboratory
     
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
     
   File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
   Code Documentation is available at: <http://doxygen.mantidproject.org>    
*/
class GEMScriptInput : public Mantid::API::Algorithm
{
public:
  /// (Empty) Constructor
  GEMScriptInput() : Mantid::API::Algorithm() {}
  /// Virtual destructor
  virtual ~GEMScriptInput() {}
  /// Algorithm's name
  virtual const std::string name() const { return "GEMScriptInput"; }
  /// Algorithm's version
  virtual const int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "DIFFRACTION"; }

private:
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

};

}
}

#endif /*MANTID_ALGORITHMS_GEMSCRIPTINPUT_H_*/

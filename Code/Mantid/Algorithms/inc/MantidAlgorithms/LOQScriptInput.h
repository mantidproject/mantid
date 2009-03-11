#ifndef MANTID_ALGORITHMS_LOQSCRIPTINPUT_H_
#define MANTID_ALGORITHMS_LOQSCRIPTINPUT_H_

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
   <LI>SampleWorkspace - The sample workspace </LI>
  <LI>EmptyCanWorkspace - The empty can workspace </LI>
  <LI>TransmissionSampleWorkspace - The sample workspace for the transmission set up </LI>
  <LI>TransmissionDirectWorkspace - The direct workspace for the transmission set up </LI>
  <LI>TransmissionEmptyCanWorkspace - The empty can workspace for the transmission set up </LI>
  <LI>EfficiencyCorrectionFile - The file to use for the efficiency correction</LI>
   </UL>
   
    Optional properties:
   <UL>
  <LI>Radius_min - The minimum radius</LI>
  <LI>Radius_max - The maximum radius</LI>
  <LI>Wavelength_min - the minimum wavelength</LI>
  <LI>Wavelength_max - The maximum wavelength</LI>
  <LI>Wavelength_delta - The wavelength binning</LI>
  <LI>Q_min - The momentum transfer minimum</LI>
  <LI>Q_max - The momentum transfer maximum</LI>
  <LI>Q_delta - The momentum trasnfer binning</LI>
  <LI>Beam_Centre_X - The x position of the beam centre</LI>
  <LI>Beam_Centre_Y - The y position of the beam centre</LI>
   </UL>
   @author Martyn Gigg, Tessella Support Services plc
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
class LOQScriptInput : public Mantid::API::Algorithm
{
public:
  /// (Empty) Constructor
  LOQScriptInput() : Mantid::API::Algorithm() {}
  /// Virtual destructor
  virtual ~LOQScriptInput() {}
  /// Algorithm's name
  virtual const std::string name() const { return "LOQScriptInput"; }
  /// Algorithm's version
  virtual const int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "SANS"; }

private:
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

  /// Static reference to the logger class
  static Mantid::Kernel::Logger& g_log;
};

}
}

#endif /*MANTID_ALGORITHMS_LOQSCRIPTINPUT_H_*/

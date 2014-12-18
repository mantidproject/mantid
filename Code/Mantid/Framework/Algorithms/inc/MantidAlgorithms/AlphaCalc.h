#ifndef MANTID_ALGORITHM_MUONALPHACALC_H_
#define MANTID_ALGORITHM_MUONALPHACALC_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace Algorithms
  {
    /**Muon algorithm for calculating the detector efficiency between two groups of detectors.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
    <LI> ForwardSpectra - The spectra numbers of the forward group </LI>
    <LI> BackwardSpectra - The spectra numbers of the backward group </LI>
    <LI> FirstGoodValue - First good value </LI>
    <LI> LastGoodValue - Last good value </LI>
    <LI> Alpha (output) </LI>
    </UL>


    @author Anders Markvardsen, ISIS, RAL
    @date 21/09/2010

    Copyright &copy; 2008-10 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport AlphaCalc : public API::Algorithm
    {
    public:
      /// Default constructor
      AlphaCalc() : API::Algorithm() {};
      /// Destructor
      virtual ~AlphaCalc() {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "AlphaCalc";}
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Muon algorithm for calculating the detector efficiency between two groups of detectors.";}

      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1;}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "Muon";}

    private:
      // Overridden Algorithm methods
      void init();
      void exec();

    };

  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_MUONALPHACALC_H_*/

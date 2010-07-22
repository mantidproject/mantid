#ifndef MANTID_CURVEFITTING_FITMULTISPECTRAL_H_
#define MANTID_CURVEFITTING_FITMULTISPECTRAL_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFunction.h"

namespace Mantid
{
  namespace CurveFitting
  {
    /**

    FitMultispectral. Fits the same 1d function to a number of spectra in 
    a workspace. Returns a Workspace2D with spectra calculated with the fitted
    parameters and a set of parameters for each sectrum in a TableWorkspace

    @author Roman Tolchenov, Tessella plc
    @date 24/03/2010

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport FitMultispectral : public API::Algorithm
    {
    public:
      /// Default constructor
      FitMultispectral() : API::Algorithm(){};
      /// Destructor
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "FitMultispectral";}
      /// Algorithm's version for identification overriding a virtual method
      virtual const int version() const { return (1);}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "CurveFitting";}

    protected:
      // Overridden Algorithm methods
      void init();
      void exec();
    };

    
  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_FITMULTISPECTRAL_H_*/

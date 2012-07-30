#ifndef MANTID_MDALGORITHMS_RESOLUTIONCONVOLVEDCROSSSECTION_H_
#define MANTID_MDALGORITHMS_RESOLUTIONCONVOLVEDCROSSSECTION_H_
/**
  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
#include "MantidAPI/IFunctionMD.h"
#include "MantidAPI/ParamFunctionAttributeHolder.h"
#include "MantidAPI/IMDEventWorkspace.h"

namespace Mantid
{
  namespace API
  {
    /// Forward declarations
    class ExperimentInfo;
  }


  namespace MDAlgorithms
  {
    //
    // Forward declarations
    //
    class MDResolutionConvolution;
    class ForegroundModel;

    /**
     * Defines a Fit function that calculates the convolution
     * of a foreground model with a resolution calculation for an MD workspace.
     */
    class DLLExport ResolutionConvolvedCrossSection :
      public virtual API::ParamFunctionAttributeHolder, public virtual API::IFunctionMD
    {
    public:
      /// Constructor
      ResolutionConvolvedCrossSection();
      /// Destructor
      ~ResolutionConvolvedCrossSection();

      /// Declare the attributes associated with this function
      void declareAttributes();
      /// Declare model parameters.
      void declareParameters();

      /// Name for the function
      std::string name() const { return "ResolutionConvolvedCrossSection"; }
      /// Set a value to a named attribute. Ensures additional parameters are set when foreground is set
      void setAttribute(const std::string& name, const API::IFunction::Attribute & value);

    private:
      /// Override the main domain function to access the workspace
      void function(const API::FunctionDomain& domain, API::FunctionValues& values) const;
      /// Return the signal contribution for the given box
      double functionMD(const API::IMDIterator & box) const;

      /// Set a pointer to the concrete convolution object
      void setupResolutionFunction(const std::string & name, const std::string & fgModelName);

      /// The meat of the calculation for each MD point
      MDResolutionConvolution *m_convolution;

      /// A pointer to the MD event workspace providing the data
      mutable API::IMDEventWorkspace_const_sptr m_workspace;
    };

  }
}


#endif /* MANTID_MDALGORITHMS_RESOLUTIONCONVOLVEDCROSSSECTION_H_ */

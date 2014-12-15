#ifndef MANTID_ALGORITHM_SCALEX_H_
#define MANTID_ALGORITHM_SCALEX_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Workspace.h"

#include <boost/function.hpp>

namespace Mantid
{
  namespace Algorithms
  {
    /**Takes a workspace and adjusts all the time bin values by the same multiplicative factor.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace to take as input </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the result </LI>
    <LI> Factor - The scaling factor to multiply the time bins by</LI>
    </UL>
      
    @author 
    @date 6/23/2011

    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class DLLExport ScaleX : public API::Algorithm
    {
    public:
      /// Default constructor
      ScaleX();
      /// Destructor
      virtual ~ScaleX();
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "ScaleX";}
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Scales the X-axis of an input workspace by the given factor, which can be either multiplicative or additive.";}

      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1;}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "Arithmetic;CorrectionFunctions";}

    private:
      
      // Overridden Algorithm methods
      void init();
      void exec();
      /// Execute algorithm for EventWorkspaces
      void execEvent();
    
      /// Create output workspace
      API::MatrixWorkspace_sptr createOutputWS(const API::MatrixWorkspace_sptr & input);
      /// Get the scale factor for the given spectrum
      double getScaleFactor(const API::MatrixWorkspace_const_sptr & inputWS, const size_t index);
       
      /// The progress reporting object
      API::Progress *m_progress;

      /// Scaling factor
      double m_algFactor;
      /// instrument parameter name
      std::string m_parname;
      /// Flag whether we are combining input parameters
      bool m_combine;
      /// Function defining request operation
      boost::function<double (double,double)> m_binOp;
      /// Start workspace index
      int m_wi_min;
      /// Stop workspace index
      int m_wi_max;
    };

  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_SCALEX_H_*/

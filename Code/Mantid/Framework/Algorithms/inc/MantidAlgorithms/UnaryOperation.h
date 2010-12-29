#ifndef MANTID_ALGORITHMS_UNARYOPERATION_H_
#define MANTID_ALGORITHMS_UNARYOPERATION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace Algorithms
  {
    /** 
    UnaryOperation supports the implementation of a Unary operation on an input workspace.
    It inherits from the Algorithm class, and overrides the init() & exec() methods.
    Concrete sub-classes should implement (or re-implement, if necessary) the protected
    methods of this class. The init() & exec() methods should be extended only in
    unusual circumstances, and NEVER overridden.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the input workspace </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the result </LI>
    </UL>

    @author Russell Taylor, Tessella plc
    @date 24/03/2009

    Copyright &copy; 2009-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    */
    class DLLExport UnaryOperation : public API::Algorithm
    {
    public:
      /// Default constructor
      UnaryOperation();
      /// Destructor
      virtual ~UnaryOperation();

      /// Algorithm's category for identification
      virtual const std::string category() const { return "CorrectionFunctions";}

    protected:
      // Overridden Algorithm methods
      virtual void init();
      virtual void exec();

      /// The name of the input workspace property
      virtual const std::string inputPropName() const { return "InputWorkspace";}
      /// The name of the output workspace property
      virtual const std::string outputPropName() const { return "OutputWorkspace";}

      /// A virtual function in which additional properties of an algorithm should be declared. Called by init().
      virtual void defineProperties() { /*Empty in base class*/ }
      /// A virtual function in which additional properties should be retrieved into member variables. Called by exec().
      virtual void retrieveProperties() { /*Empty in base class*/ }

      /** Carries out the Unary operation on the current 'cell'
       *  @param XIn The X value. This will be the bin centre for histogram workspaces.
       *  @param YIn The input data value
       *  @param EIn The input error value
       *  @param YOut A reference to the output data
       *  @param EOut A reference to the output error
       */
      virtual void performUnaryOperation(const double XIn, const double YIn, const double EIn, double& YOut, double& EOut) = 0;
    };

  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_UNARYOPERATION_H_*/

#ifndef MANTID_ALGORITHMS_POISSONERRORS_H_
#define MANTID_ALGORITHMS_POISSONERRORS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/LocatedDataRef.h" 
#include "MantidAlgorithms/BinaryOperation.h"

namespace Mantid
{
namespace Algorithms
{
/** Takes a Data workspace and an original counts workspace input and updates the 
    error values in the data workspace to be the same fractionally as the counts workspace.
		The number of histograms, the binning and units of the two workspaces must match.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace2D to take as input</LI>
    <LI> CountsWorkspace - The name of the workspace that contains the original counts </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the result </LI>
    </UL>

    @author Nick Draper, Tessella Support Services plc
    @date 03/02/2009

    Copyright &copy; 2007-9 STFC Rutherford Appleton Laboratory

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
class DLLExport PoissonErrors : public BinaryOperation
{
public:
  /// Default constructor
  PoissonErrors() : BinaryOperation() {};
  /// Destructor
  virtual ~PoissonErrors() {};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "PoissonErrors";}
  /// Algorithm's version for identification overriding a virtual method
  virtual const int version() const { return (1);}
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "SANS";}

private:

// Overridden BinaryOperation methods
  void performBinaryOperation(API::MatrixWorkspace::const_iterator it_in1, API::MatrixWorkspace::const_iterator it_in2,
    API::MatrixWorkspace::iterator it_out);
	virtual const bool checkSizeCompatibility(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs) const;
  
  /// The name of the first input workspace property for BinaryOperation
	virtual const std::string inputPropName1() const { return "InputWorkspace";}
  /// The name of the second input workspace property for BinaryOperation
	virtual const std::string inputPropName2() const { return "CountsWorkspace";}
  /// The name of the output workspace property for BinaryOperation
	virtual const std::string outputPropName() const { return "OutputWorkspace";}

	/// Internal class providing the binary function
  class PoissonErrors_fn : public BinaryOperation::BinaryOperation_fn
  {
  public:
    /// Constructor
    PoissonErrors_fn(BinaryOperation *op,int count) : BinaryOperation::BinaryOperation_fn(op,count) {}
    API::LocatedDataValue& operator()(const API::ILocatedData&,const API::ILocatedData&);
  };

  /// Static reference to the logger class
  static Mantid::Kernel::Logger& g_log;
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_POISSONERRORS_H_*/

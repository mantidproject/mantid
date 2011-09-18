#ifndef MANTID_ALGORITHMS_REBIN_H_
#define MANTID_ALGORITHMS_REBIN_H_
/*WIKI* 

The algorithm rebins data with new bin boundaries. The 'params' property defines new boundaries in intervals <math>x_i-x_{i+1}\,</math>. Positive <math>\Delta x_i\,</math> make constant width bins, whilst negative ones create logarithmic binning using the formula <math>x(j+1)=x(j)(1+|\Delta x_i|)\,</math>

This algorithms is useful both in data reduction, but also in remapping [[Ragged Workspace|ragged workspaces]] to a regular set of bin boundaries.

The bin immediately before the specified boundaries <math>x_2</math>, <math>x_3</math>, ... <math>x_i</math> is likely to have a different width from its neighbours because there can be no gaps between bins. Rebin ensures that any of these space filling bins cannot be less than 25% or more than 125% of the width that was specified.

=== Example Rebin param strings ===
;0,100,20000
:From 0 rebin in constant size bins of 100 up to 20,000
;2,-0.035,10
:From 10 rebin in Logarithmic bins of 0.035 up to 10
;0,100,10000,200,20000
:From 0 rebin in steps of 100 to 10,000 then steps of 200 to 20,000

=== For EventWorkspaces ===

If the input is an [[EventWorkspace]] and the "Preserve Events" property is True, the rebinning is performed in place, and only the X axes of the workspace are set. The actual Y histogram data will only be requested as needed, for example, when plotting or displaying the data. 

If "Preserve Events" is false., then the output workspace will be created as a [[Workspace2D]], with fixed histogram bins, and all Y data will be computed immediately. All event-specific data is lost at that point.

=== For Data-Point Workspaces ===

If the input workspace contains data points, rather than histograms, then Rebin will automatically use the [[ConvertToHistogram]] and [[ConvertToPointData]] algorithms before and after the rebinning has taken place.

== Usage ==
'''Python'''
 Rebin("InWS2","OutWS","x1,dx1,x2")


*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/** Takes a workspace as input and rebins the data according to the input rebin parameters.

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The name of the workspace to take as input. Must contain histogram data. </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the result. </LI>
    <LI> RebinParameters - The new bin boundaries in the form X1,deltaX1,X2,deltaX2,X3,... </LI>
    </UL>

    The algorithms used in the VectorHelper::rebin() and VectorHelper::createAxisFromRebinParams() 
    are based on the algorithms used in OPENGENIE /src/transform_utils.cxx (Freddie Akeroyd, ISIS).
    When calculating the bin boundaries, if the last bin ends up with a width being less than 25%
    of the penultimate one, then the two are combined. 

    @author Dickon Champion, STFC
    @date 25/02/2008

    Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport Rebin : public API::Algorithm
{
public:
  /// Default constructor
  Rebin() : API::Algorithm() {};
  /// Destructor
  virtual ~Rebin() {};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "Rebin";}
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1;}
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Rebin";}
  /// Algorithm's aliases
  virtual const std::string alias() const { return "rebin"; }

protected:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  // Overridden Algorithm methods
  void init();
  virtual void exec();
  
  void propagateMasks(API::MatrixWorkspace_const_sptr inputW, API::MatrixWorkspace_sptr outputW, int hist);
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_REBIN_H_*/

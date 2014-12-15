#ifndef MANTID_ALGORITHMS_SOLIDANDGLE_H_
#define MANTID_ALGORITHMS_SOLIDANDGLE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/** Calculates and outputs the solid angles for each detector in the instrument.  
    The sample position is taken as a point source for the solid angle calculations.

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input workspace. </LI>
    <LI> OutputWorkspace - The name of the output workspace. Can be the same as the input one. </LI>
    </UL>

    @author Nick Draper, Tessella Support Services plc
    @date 26/01/2009

    Copyright &copy; 2009-2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class DLLExport SolidAngle : public API::Algorithm
{
public:
  /// Default constructor
  SolidAngle();
  /// Virtual destructor
  virtual ~SolidAngle();
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "SolidAngle"; }
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "The SolidAngle algorithm calculates the solid angle in steradians for each of the detectors in an instrument and outputs the data in a workspace.  This can then be used to normalize a data workspace using the divide algorithm should you wish.";}

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "CorrectionFunctions\\InstrumentCorrections";}

private:
  
  // Overridden Algorithm methods
  void init();
  void exec();
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SOLIDANDGLE_H_*/

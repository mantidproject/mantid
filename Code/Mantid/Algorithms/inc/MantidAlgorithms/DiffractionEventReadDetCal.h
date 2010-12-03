#ifndef MANTID_ALGORITHMS_DIFFRACTIONEVENTREADDETCAL_H_
#define MANTID_ALGORITHMS_DIFFRACTIONEVENTREADDETCAL_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_multifit_nlin.h>
#include <gsl/gsl_multimin.h>
#include <gsl/gsl_blas.h>

namespace Mantid
{
namespace Algorithms
{
/**
 Find the offsets for each detector

 @author Vickie Lynch, SNS, ORNL
 @date 12/02/2010

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport DiffractionEventReadDetCal: public API::Algorithm
{
public:
  /// Default constructor
  DiffractionEventReadDetCal();
  /// Destructor
  virtual ~DiffractionEventReadDetCal();
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "DiffractionEventReadDetCal"; }
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Diffraction"; }
  /// Function to optimize
  void intensity(double x, double y, double z, double ax, double ay, double az, double angle, std::string detname, std::string inname);
private:
  // Overridden Algorithm methods
  void init();
  void exec();
  // Matrix workspace pointer
  //API::MatrixWorkspace_sptr inputW;

};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHM_DIFFRACTIONEVENTREADDETCAL_H_*/

#ifndef MANTID_ALGORITHMS_DIFFRACTIONEVENTCALIBRATEDETECTORS_H_
#define MANTID_ALGORITHMS_DIFFRACTIONEVENTCALIBRATEDETECTORS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include <gsl/gsl_blas.h>
#include <gsl/gsl_multifit_nlin.h>
#include <gsl/gsl_multimin.h>
#include <gsl/gsl_statistics.h>

namespace Mantid {
namespace Algorithms {
/**
 Find the offsets for each detector

 @author Vickie Lynch SNS, ORNL
 @date 12/02/2010

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
 National Laboratory & European Spallation Source

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
class DLLExport DiffractionEventCalibrateDetectors : public API::Algorithm {
public:
  /// Default constructor
  DiffractionEventCalibrateDetectors();
  /// Destructor
  virtual ~DiffractionEventCalibrateDetectors();
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const {
    return "DiffractionEventCalibrateDetectors";
  }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "This algorithm optimizes the position and angles of all of the "
           "detector panels. The target instruments for this feature are SNAP "
           "and TOPAZ.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const {
    return "Diffraction;CorrectionFunctions\\InstrumentCorrections";
  }
  /// Function to optimize
  double intensity(double x, double y, double z, double rotx, double roty,
                   double rotz, std::string detname, std::string inname,
                   std::string outname, std::string peakOpt,
                   std::string rb_param, std::string groupWSName);
  void movedetector(double x, double y, double z, double rotx, double roty,
                    double rotz, std::string detname,
                    Mantid::DataObjects::EventWorkspace_sptr inputW);

private:
  // Overridden Algorithm methods
  void init();
  void exec();
  // Event workspace pointer
  // API::EventWorkspace_sptr inputW;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHM_DIFFRACTIONEVENTCALIBRATEDETECTORS_H_*/

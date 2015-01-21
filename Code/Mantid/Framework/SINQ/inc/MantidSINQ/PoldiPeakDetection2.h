#ifndef MANTID_POLDI_PoldiPeakDetection2_H_
#define MANTID_POLDI_PoldiPeakDetection2_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidSINQ/DllConfig.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"

#include <vector>

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class Workspace2D_sptr;

namespace Mantid {
namespace Poldi {
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------

// N.B. PoldiPeakDetection2 is used to detecte peaks on the carrelated Poldi
// data
/** @class PoldiPeakDetection2 PoldiPeakDetection2.h Poldi/PoldiPeakDetection2.h

    Part of the Poldi scripts set, used to to detecte peaks on the carrelated
   Poldi data

    @author Christophe Le Bourlot, Paul Scherrer Institut - SINQ
    @date 05/06/2013

    Copyright © 2013 PSI-MSS

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

    File change history is stored at:
   <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at <http://doxygen.mantidproject.org>
*/
class MANTID_SINQ_DLL PoldiPeakDetection2 : public API::Algorithm {
public:
  /// Default constructor
  PoldiPeakDetection2(){};
  /// Destructor
  virtual ~PoldiPeakDetection2() {}
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "PoldiPeakDetection"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Peak detection used on a diffractogram, with peak refinement thru "
           "a peak fit with a gaussian function.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 2; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "SINQ\\Poldi\\Obsolete"; }

protected:
  /// Overwrites Algorithm method
  void exec();

private:
  /// Overwrites Algorithm method.
  void init();

  // return the min between two integer
  inline size_t min(size_t a, size_t b) { return (a < b) ? a : b; }
  // return the max between two integer
  inline size_t max(size_t a, size_t b) { return (a > b) ? a : b; }

  // return the index of maximal intensity
  int getIndexOfMax();

  // copy from ..... algorithm
  // do the fit of one single peak
  bool doFitGaussianPeak(DataObjects::Workspace2D_sptr dataws,
                         int workspaceindex, double &center, double &sigma,
                         double &height, double xmin, double xmax);

  // the output workspace to store the correlated function
  DataObjects::Workspace2D_sptr ws_auto_corr;
  // number of x channel of the correlated function (in the d-space)
  size_t nb_d_channel;
  // table of dead wires not to take into account
  std::vector<bool> table_dead_wires;
};

} // namespace Poldi
} // namespace Mantid

#endif /*MANTID_POLDI_PoldiPeakDetection2_H_*/

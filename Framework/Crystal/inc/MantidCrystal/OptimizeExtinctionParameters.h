#ifndef MANTID_CRYSTAL_OptimizeExtinctionParameters_H_
#define MANTID_CRYSTAL_OptimizeExtinctionParameters_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidGeometry/Crystal/PointGroup.h"
#include <gsl/gsl_blas.h>
#include <gsl/gsl_multifit_nlin.h>
#include <gsl/gsl_multimin.h>
#include <gsl/gsl_statistics.h>

namespace Mantid {
namespace Algorithms {
/**
 Find the offsets for each detector

 @author Vickie Lynch, SNS
 @date 02/06/2012

 Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport OptimizeExtinctionParameters : public API::Algorithm {
public:
  /// Default constructorMatrix
  OptimizeExtinctionParameters();
  /// Destructor
  virtual ~OptimizeExtinctionParameters();
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const {
    return "OptimizeExtinctionParameters";
  }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Finds optimal mosaic and r_crystallite parameters for extinction "
           "correction.";
  }
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Crystal"; }
  /// Call TOFExtinction as a Child Algorithm to get statistics of the peaks
  double fitMosaic(double mosaic, double rcrystallite, std::string inname,
                   std::string corrOption, std::string pointOption,
                   std::string tofParams);

private:
  /// Point Groups possible
  std::vector<Mantid::Geometry::PointGroup_sptr> m_pointGroups;
  // Overridden Algorithm methods
  void init();
  void exec();
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_CRYSTAL_OptimizeExtinctionParameters_H_*/

#ifndef MANTID_ALGORITHMS_REBIN2D_H_
#define MANTID_ALGORITHMS_REBIN2D_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Math/Quadrilateral.h"
#include "MantidDataObjects/RebinnedOutput.h"

namespace Mantid {
//------------------------------------------------------------------------------
// Forward declarations
//------------------------------------------------------------------------------
namespace Geometry {
class ConvexPolygon;
}

namespace Algorithms {

/**
Rebins both axes of a two-dimensional workspace to the given parameters.

@author Martyn Gigg, Tessella plc

Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport Rebin2D : public API::Algorithm {
public:
  /// Algorithm's name for identification
  virtual const std::string name() const { return "Rebin2D"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Rebins both axes of a 2D workspace using the given parameters";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Transforms\\Rebin"; }

protected:
  /// Rebin the input quadrilateral to to output grid
  void rebinToOutput(const Geometry::Quadrilateral &inputQ,
                     API::MatrixWorkspace_const_sptr inputWS, const size_t i,
                     const size_t j, API::MatrixWorkspace_sptr outputWS,
                     const std::vector<double> &verticalAxis);
  /// Rebin the input quadrilateral to to output grid
  void rebinToFractionalOutput(const Geometry::Quadrilateral &inputQ,
                               API::MatrixWorkspace_const_sptr inputWS,
                               const size_t i, const size_t j,
                               DataObjects::RebinnedOutput_sptr outputWS,
                               const std::vector<double> &verticalAxis);

  /// Find the intersect region on the output grid
  bool getIntersectionRegion(API::MatrixWorkspace_const_sptr outputWS,
                             const std::vector<double> &verticalAxis,
                             const Geometry::Quadrilateral &inputQ,
                             size_t &qstart, size_t &qend, size_t &en_start,
                             size_t &en_end) const;
  /// Compute sqrt of errors and put back in bin width division if necessary
  void normaliseOutput(API::MatrixWorkspace_sptr outputWS,
                       API::MatrixWorkspace_const_sptr inputWS);

  /// Progress reporter
  boost::shared_ptr<API::Progress> m_progress;

private:
  /// Flag for using a RebinnedOutput workspace
  bool useFractionalArea;

  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();
  /// Setup the output workspace
  API::MatrixWorkspace_sptr
  createOutputWorkspace(API::MatrixWorkspace_const_sptr parent,
                        MantidVec &newXBins, MantidVec &newYBins) const;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_REBIN2D_H_ */

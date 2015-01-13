#ifndef MANTID_ALGORITHMS_STITCH1D_H_
#define MANTID_ALGORITHMS_STITCH1D_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include <boost/tuple/tuple.hpp>

namespace Mantid {
namespace Algorithms {

/** Stitch1D : Stitches two Matrix Workspaces together into a single output.

 Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport Stitch1D : public API::Algorithm {
public:
  /// Default constructor
  Stitch1D(){};
  /// Destructor
  virtual ~Stitch1D(){};
  /// Algorithm's name for identification. @see Algorithm::name
  virtual const std::string name() const { return "Stitch1D"; }
  /// Algorithm's version for identification. @see Algorithm::version
  virtual int version() const { return 3; }
  /// Algorithm's category for identification. @see Algorithm::category
  virtual const std::string category() const { return "Reflectometry"; }
  /// Summary of algorithm's purpose
  virtual const std::string summary() const {
    return "Stitches single histogram matrix workspaces together";
  }
  /// Does the x-axis have non-zero errors
  bool hasNonzeroErrors(Mantid::API::MatrixWorkspace_sptr ws);

private:
  /// Helper typedef. For storing indexes of special values per spectra per
  /// workspace.
  typedef std::vector<std::vector<size_t>> SpecialTypeIndexes;
  /// Overwrites Algorithm method.
  void init();
  /// Overwrites Algorithm method.
  void exec();
  /// Get the start overlap
  double getStartOverlap(const double &intesectionMin,
                         const double &intesectionMax) const;
  /// Get the end overlap
  double getEndOverlap(const double &intesectionMin,
                       const double &intesectionMax) const;

  /// Get the rebin parameters
  Mantid::MantidVec getRebinParams(Mantid::API::MatrixWorkspace_sptr &lhsWS,
                                   Mantid::API::MatrixWorkspace_sptr &rhsWS,
                                   const bool scaleRHSWS) const;
  /// Perform rebin
  Mantid::API::MatrixWorkspace_sptr
  rebin(Mantid::API::MatrixWorkspace_sptr &input,
        const Mantid::MantidVec &params);
  /// Perform integration
  Mantid::API::MatrixWorkspace_sptr
  integration(Mantid::API::MatrixWorkspace_sptr &input, const double &start,
              const double &stop);
  /// Perform multiplication over a range
  Mantid::API::MatrixWorkspace_sptr
  multiplyRange(Mantid::API::MatrixWorkspace_sptr &input, const int &startBin,
                const int &endBin, const double &factor);
  /// Perform multiplication over a range
  Mantid::API::MatrixWorkspace_sptr
  multiplyRange(Mantid::API::MatrixWorkspace_sptr &input, const int &startBin,
                const double &factor);
  /// Create a single valued workspace
  Mantid::API::MatrixWorkspace_sptr singleValueWS(double val);
  /// Calclate the weighted mean
  Mantid::API::MatrixWorkspace_sptr
  weightedMean(Mantid::API::MatrixWorkspace_sptr &inOne,
               Mantid::API::MatrixWorkspace_sptr &inTwo);
  /// Find the start and end indexes
  boost::tuple<int, int>
  findStartEndIndexes(double startOverlap, double endOverlap,
                      Mantid::API::MatrixWorkspace_sptr &workspace);
  /// Mask out everything but the data in the ranges
  Mantid::API::MatrixWorkspace_sptr
  maskAllBut(int a1, int a2, Mantid::API::MatrixWorkspace_sptr &source);
  /// Mask out everything but the data in the ranges, but do it inplace.
  void maskInPlace(int a1, int a2, Mantid::API::MatrixWorkspace_sptr source);
  /// Add back in any special values
  void reinsertSpecialValues(Mantid::API::MatrixWorkspace_sptr ws);
  /// Range tolerance
  static const double range_tolerance;
  /// Index per workspace spectra of Nans
  SpecialTypeIndexes m_nanYIndexes;
  /// Index per workspace spectra of Infs
  SpecialTypeIndexes m_infYIndexes;
  /// Index per workspace spectra of Nans
  SpecialTypeIndexes m_nanEIndexes;
  /// Index per workspace spectra of Infs
  SpecialTypeIndexes m_infEIndexes;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_STITCH1D_H_ */

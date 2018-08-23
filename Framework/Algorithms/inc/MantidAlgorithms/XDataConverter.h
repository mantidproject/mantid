#ifndef MANTID_ALGORITHMS_XDATACONVERTER_H_
#define MANTID_ALGORITHMS_XDATACONVERTER_H_

#include "MantidAPI/DistributedAlgorithm.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {
namespace HistogramData {
class HistogramDx;
class HistogramX;
} // namespace HistogramData
namespace Algorithms {
/**
  This is an abstract base class for sharing methods between algorithms that
  operate only
  on X data. Inheriting classes should overide the isRequired,
  checkInputWorkspace, getNewXSize and
  setXData methods to return the appropriate values.

  @author Martyn Gigg, Tessella plc
  @date 2010-12-14

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
class DLLExport XDataConverter : public API::DistributedAlgorithm {
public:
  /// Default constructor
  XDataConverter();
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "General"; }

protected:
  /// Returns true if the algorithm needs to be run.
  virtual bool
  isProcessingRequired(const API::MatrixWorkspace_sptr inputWS) const = 0;
  /// Returns the size of the new X vector
  virtual std::size_t getNewXSize(const std::size_t ySize) const = 0;
  /// Calculate the X point values. Implement in an inheriting class.
  virtual Kernel::cow_ptr<HistogramData::HistogramX> calculateXPoints(
      const Kernel::cow_ptr<HistogramData::HistogramX> inputX) const = 0;

private:
  /// Override init
  void init() override;
  /// Override exec
  void exec() override;

  std::size_t getNewYSize(const API::MatrixWorkspace_sptr inputWS);

  /// Set the X data on given spectra
  void setXData(API::MatrixWorkspace_sptr outputWS,
                const API::MatrixWorkspace_sptr inputWS, const int index);

  /// Flag if the X data is shared
  bool m_sharedX;
  /// Cached data for shared X values
  Kernel::cow_ptr<HistogramData::HistogramX> m_cachedX{nullptr};
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_XDATACONVERTER_H_*/

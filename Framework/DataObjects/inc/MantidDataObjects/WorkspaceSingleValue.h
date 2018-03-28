#ifndef MANTID_DATAOBJECTS_WORKSPACESINGLEVALUE_H_
#define MANTID_DATAOBJECTS_WORKSPACESINGLEVALUE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/HistoWorkspace.h"
#include "MantidDataObjects/Histogram1D.h"

namespace Mantid {

namespace DataObjects {
/** Concrete workspace implementation. Data is a single double value
    @author Nicholas Draper
    @date 19/05/2008

    Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport WorkspaceSingleValue : public API::HistoWorkspace {
public:
  /**	Gets the name of the workspace type
   * @return Standard string name  */
  const std::string id() const override { return "WorkspaceSingleValue"; }

  WorkspaceSingleValue(
      double value = 0.0, double error = 0.0,
      const Parallel::StorageMode storageMode = Parallel::StorageMode::Cloned);

  /// Returns a clone of the workspace
  std::unique_ptr<WorkspaceSingleValue> clone() const {
    return std::unique_ptr<WorkspaceSingleValue>(doClone());
  }
  /// Returns a default-initialized clone of the workspace
  std::unique_ptr<WorkspaceSingleValue> cloneEmpty() const {
    return std::unique_ptr<WorkspaceSingleValue>(doCloneEmpty());
  }
  WorkspaceSingleValue &operator=(const WorkspaceSingleValue &other) = delete;
  /// Returns the number of single indexable items in the workspace
  std::size_t size() const override { return 1; }

  /// Returns the size of each block of data returned by the dataX accessors
  std::size_t blocksize() const override { return 1; }

  /// @return the number of histograms (spectra)
  std::size_t getNumberHistograms() const override { return 1; }

  Histogram1D &getSpectrum(const size_t index) override;
  const Histogram1D &getSpectrum(const size_t index) const override;

  void generateHistogram(const std::size_t index, const MantidVec &X,
                         MantidVec &Y, MantidVec &E,
                         bool skipError = false) const override;

  /// Returns the number of dimensions, 0 in this case.
  size_t getNumDims() const override;

protected:
  /// Protected copy constructor. May be used by childs for cloning.
  WorkspaceSingleValue(const WorkspaceSingleValue &other);

private:
  WorkspaceSingleValue *doClone() const override {
    return new WorkspaceSingleValue(*this);
  }
  WorkspaceSingleValue *doCloneEmpty() const override {
    return new WorkspaceSingleValue();
  }

  // allocates space in a new workspace - does nothing in this case
  void init(const std::size_t &NVectors, const std::size_t &XLength,
            const std::size_t &YLength) override;
  void init(const HistogramData::Histogram &histogram) override;

  /// Instance of Histogram1D that holds the "spectrum" (AKA the single value);
  Histogram1D data{HistogramData::Histogram::XMode::Points,
                   HistogramData::Histogram::YMode::Counts};
};

/// shared pointer to the WorkspaceSingleValue class
using WorkspaceSingleValue_sptr = boost::shared_ptr<WorkspaceSingleValue>;
using WorkspaceSingleValue_const_sptr =
    boost::shared_ptr<const WorkspaceSingleValue>;

} // namespace DataObjects
} // namespace Mantid

#endif /*MANTID_DATAOBJECTS_WORKSPACESINGLEVALUE_H_*/

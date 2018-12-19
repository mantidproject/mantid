// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/IPropertyManager.h"

namespace Mantid {
namespace DataObjects {

using std::size_t;

DECLARE_WORKSPACE(WorkspaceSingleValue)

/// Constructor
WorkspaceSingleValue::WorkspaceSingleValue(
    double value, double error, const Parallel::StorageMode storageMode)
    : API::HistoWorkspace(storageMode) {
  initialize(1, 1, 1);
  // Set the "histogram" to the single value
  data.dataX().resize(1, 0.0);
  data.setCounts(1, value);
  data.setCountStandardDeviations(1, error);
  data.setPointStandardDeviations(1, 0.0);

  setDistribution(true);
}

WorkspaceSingleValue::WorkspaceSingleValue(const WorkspaceSingleValue &other)
    : HistoWorkspace(other), data(other.data) {
  setDistribution(true);
}

/** Does nothing in this case
 *  @param NVectors :: This value can only be equal to one, otherwise exception
 * is thrown
 *  @param XLength :: The number of X data points/bin boundaries
 *  @param YLength :: The number of data/error points
 */
void WorkspaceSingleValue::init(const std::size_t &NVectors,
                                const std::size_t &XLength,
                                const std::size_t &YLength) {
  (void)NVectors;
  (void)XLength;
  (void)YLength; // Avoid compiler warning
}

void WorkspaceSingleValue::init(const HistogramData::Histogram &histogram) {
  UNUSED_ARG(histogram);
}

/// Return the underlying Histogram1D at the given workspace index.
Histogram1D &
WorkspaceSingleValue::getSpectrumWithoutInvalidation(const size_t /*index*/) {
  data.setMatrixWorkspace(this, 0);
  return data;
}

/// Rebin the workspace. Not implemented for this workspace.
void WorkspaceSingleValue::generateHistogram(const std::size_t index,
                                             const MantidVec &X, MantidVec &Y,
                                             MantidVec &E,
                                             bool skipError) const {
  UNUSED_ARG(index);
  UNUSED_ARG(X);
  UNUSED_ARG(Y);
  UNUSED_ARG(E);
  UNUSED_ARG(skipError);
  throw std::runtime_error(
      "generateHistogram() not implemented for WorkspaceSingleValue.");
}

/// Our parent MatrixWorkspace has hardcoded 2, but we need 0.
size_t WorkspaceSingleValue::getNumDims() const { return 0; }

} // namespace DataObjects
} // namespace Mantid

namespace Mantid {
namespace Kernel {
template <>
DLLExport Mantid::DataObjects::WorkspaceSingleValue_sptr
IPropertyManager::getValue<Mantid::DataObjects::WorkspaceSingleValue_sptr>(
    const std::string &name) const {
  PropertyWithValue<Mantid::DataObjects::WorkspaceSingleValue_sptr> *prop =
      dynamic_cast<
          PropertyWithValue<Mantid::DataObjects::WorkspaceSingleValue_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected shared_ptr<WorkspaceSingleValue>.";
    throw std::runtime_error(message);
  }
}

template <>
DLLExport Mantid::DataObjects::WorkspaceSingleValue_const_sptr
IPropertyManager::getValue<
    Mantid::DataObjects::WorkspaceSingleValue_const_sptr>(
    const std::string &name) const {
  PropertyWithValue<Mantid::DataObjects::WorkspaceSingleValue_sptr> *prop =
      dynamic_cast<
          PropertyWithValue<Mantid::DataObjects::WorkspaceSingleValue_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return prop->operator()();
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected const shared_ptr<WorkspaceSingleValue>.";
    throw std::runtime_error(message);
  }
}

} // namespace Kernel
} // namespace Mantid

///\endcond TEMPLATE

// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidParallel/IO/EventLoaderHelpers.h"

namespace Mantid {
namespace Parallel {
namespace IO {
namespace EventLoader {

std::vector<size_t> readBankSizes(const H5::Group &group,
                                  const std::vector<std::string> &bankNames) {
  std::vector<size_t> bankSizes;
  for (const auto &bankName : bankNames) {
    const H5::DataSet dataset = group.openDataSet(bankName + "/event_id");
    const H5::DataSpace dataSpace = dataset.getSpace();
    bankSizes.push_back(dataSpace.getSelectNpoints());
  }
  return bankSizes;
}

H5::DataType readDataType(const H5::Group &group,
                          const std::vector<std::string> &bankNames,
                          const std::string &name) {
  return group.openDataSet(bankNames.front() + "/" + name).getDataType();
}

} // namespace EventLoader
} // namespace IO
} // namespace Parallel
} // namespace Mantid

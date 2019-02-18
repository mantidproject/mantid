// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_ROWPROCESSINGALGORITHM_H_
#define MANTID_CUSTOMINTERFACES_ROWPROCESSINGALGORITHM_H_

#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include <boost/optional.hpp>
#include <map>
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
class Batch;
class Row;
class IConfiguredAlgorithm;

MantidQt::API::IConfiguredAlgorithm_sptr
createConfiguredAlgorithm(Batch const &model, Row &row);
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTID_CUSTOMINTERFACES_ROWPROCESSINGALGORITHM_H_

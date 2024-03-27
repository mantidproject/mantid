// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "SingleFunctionTemplateModel.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MSDFunctionModel : public SingleFunctionTemplateModel {

public:
  MSDFunctionModel();
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
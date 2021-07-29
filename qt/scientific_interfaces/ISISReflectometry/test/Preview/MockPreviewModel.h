// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IPreviewModel.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <gmock/gmock.h>

#include <string>

using Mantid::API::MatrixWorkspace_sptr;

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class MockPreviewModel : public IPreviewModel {
public:
  MOCK_METHOD(void, loadWorkspace, (std::string const &), (override));
  MOCK_METHOD(MatrixWorkspace_sptr, getInstViewWorkspace, (), (const, override));
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry

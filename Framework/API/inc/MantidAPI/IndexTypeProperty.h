// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidKernel/PropertyWithValue.h"

namespace Mantid {
namespace API {

enum class IndexType { SpectrumNum = 1, WorkspaceIndex = 2 };

/** IndexTypeProperty : Implementation of a property which stores the type of
input indices users require for accessing workspace data. This works in harmony
with IndexProperty and is not intended for isolated use.

@author Lamar Moore
@date 05-05-2017
*/
class MANTID_API_DLL IndexTypeProperty : public Kernel::PropertyWithValue<std::string> {
public:
  IndexTypeProperty(const std::string &name = "IndexType", int indexType = static_cast<int>(IndexType::WorkspaceIndex));

  IndexType selectedType() const;

  int allowedTypes() const;

  std::vector<std::string> allowedValues() const override;

  bool isMultipleSelectionAllowed() override;

  using PropertyWithValue<std::string>::operator=;

  IndexTypeProperty &operator=(API::IndexType type);

  static std::string generatePropertyName(const std::string &name = "");

private:
  std::vector<std::string> m_allowedValues;
};

} // namespace API
} // namespace Mantid

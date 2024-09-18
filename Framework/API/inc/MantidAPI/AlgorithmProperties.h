// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IAlgorithmRuntimeProps.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidKernel/Strings.h"

#include <boost/optional.hpp>
#include <map>
#include <optional>
#include <string>

namespace Mantid::API::AlgorithmProperties {

// These convenience functions convert properties of various types into
// strings to set the relevant property in an AlgorithmRuntimeProps

void MANTID_API_DLL update(std::string const &property, std::string const &value, IAlgorithmRuntimeProps &properties);
// TODO this method should no longer exist - migrate code to std::optional
void MANTID_API_DLL update(std::string const &property, boost::optional<std::string> const &value,
                           IAlgorithmRuntimeProps &properties);
void MANTID_API_DLL update(std::string const &property, std::optional<std::string> const &value,
                           IAlgorithmRuntimeProps &properties);

void MANTID_API_DLL update(std::string const &property, bool value, IAlgorithmRuntimeProps &properties);

void MANTID_API_DLL update(std::string const &property, int value, IAlgorithmRuntimeProps &properties);

void MANTID_API_DLL update(std::string const &property, size_t value, IAlgorithmRuntimeProps &properties);

void MANTID_API_DLL update(std::string const &property, double value, IAlgorithmRuntimeProps &properties);

// TODO this method should no longer exist - migrate code to std::optional
void MANTID_API_DLL update(std::string const &property, boost::optional<double> const &value,
                           IAlgorithmRuntimeProps &properties);
void MANTID_API_DLL update(std::string const &property, std::optional<double> const &value,
                           IAlgorithmRuntimeProps &properties);

void MANTID_API_DLL update(std::string const &property, Workspace_sptr const &workspace,
                           IAlgorithmRuntimeProps &properties);

void MANTID_API_DLL update(std::string const &property, MatrixWorkspace_sptr const &workspace,
                           IAlgorithmRuntimeProps &properties);

void MANTID_API_DLL update(std::string const &property, IFunction_sptr const &function,
                           IAlgorithmRuntimeProps &properties);

void MANTID_API_DLL updateFromMap(IAlgorithmRuntimeProps &properties,
                                  std::map<std::string, std::string> const &parameterMap);

std::string MANTID_API_DLL getOutputWorkspace(const Mantid::API::IAlgorithm_sptr &algorithm,
                                              std::string const &property);

template <typename VALUE_TYPE>
void update(std::string const &property, std::vector<VALUE_TYPE> const &values,
            Mantid::API::IAlgorithmRuntimeProps &properties, bool const convertToString = true) {
  if (values.size() < 1)
    return;

  if (convertToString) {
    auto value = Mantid::Kernel::Strings::simpleJoin(values.cbegin(), values.cend(), ", ");
    update(property, value, properties);
  } else {
    properties.setProperty(property, values);
  }
}

} // namespace Mantid::API::AlgorithmProperties

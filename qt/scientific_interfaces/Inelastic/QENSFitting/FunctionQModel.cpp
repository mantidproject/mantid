// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FunctionQModel.h"

#include <utility>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/TextAxis.h"
#include "MantidKernel/Logger.h"

using namespace Mantid::API;

namespace {

Mantid::Kernel::Logger logger("FunctionQModel");

} // namespace

namespace MantidQt::CustomInterfaces::Inelastic {

FunctionQModel::FunctionQModel() { m_fitType = FUNCTIONQ_STRING; }

std::string FunctionQModel::getResultXAxisUnit() const { return ""; }

std::string FunctionQModel::getResultLogName() const { return "SourceName"; }

} // namespace MantidQt::CustomInterfaces::Inelastic

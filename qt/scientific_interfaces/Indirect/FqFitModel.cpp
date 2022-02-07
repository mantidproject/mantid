// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FqFitModel.h"

#include <utility>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/TextAxis.h"
#include "MantidKernel/Logger.h"

using namespace Mantid::API;

namespace {

Mantid::Kernel::Logger logger("FqFitModel");

} // namespace

namespace MantidQt::CustomInterfaces::IDA {

FqFitModel::FqFitModel() { m_fitType = FQFIT_STRING; }

std::string FqFitModel::getResultXAxisUnit() const { return ""; }

std::string FqFitModel::getResultLogName() const { return "SourceName"; }

} // namespace MantidQt::CustomInterfaces::IDA

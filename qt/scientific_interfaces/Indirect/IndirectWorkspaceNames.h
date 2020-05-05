// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <string>
#include <utility>

#include "DllConfig.h"
#include "IndexTypes.h"
#include "IndirectFitdata.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
static const std::string SIM_STRING = "sim";
static const std::string SEQ_STRING = "seq";
static const std::string IQTFIT_STRING = "IQt";
static const std::string CONVFIT_STRING = "Conv";
static const std::string MSDFIT_STRING = "Msd";
static const std::string FQFIT_STRING = "FQ";
static const std::string MULTI_STRING = "Multi";
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
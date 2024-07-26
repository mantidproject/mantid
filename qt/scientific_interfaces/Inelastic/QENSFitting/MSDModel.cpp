// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MSDModel.h"

using namespace Mantid::API;

namespace MantidQt::CustomInterfaces::Inelastic {

MSDModel::MSDModel() { m_fitType = MSD_STRING; }

std::string MSDModel::getResultXAxisUnit() const { return "Temperature"; }

} // namespace MantidQt::CustomInterfaces::Inelastic

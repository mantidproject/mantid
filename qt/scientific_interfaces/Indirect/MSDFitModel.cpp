// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MSDFitModel.h"

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

MSDFitModel::MSDFitModel() { m_fitType = MSDFIT_STRING; }

std::string MSDFitModel::singleFitOutputName(TableDatasetIndex index,
                                             WorkspaceIndex spectrum) const {
  return createSingleFitOutputName(
      "%1%_MSDFit_" + fitModeToName[getFittingMode()] + "_s%2%_Results", index,
      spectrum);
}

std::string MSDFitModel::getResultXAxisUnit() const { return "Temperature"; }

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

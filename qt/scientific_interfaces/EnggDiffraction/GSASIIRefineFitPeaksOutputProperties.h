// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "RunLabel.h"

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <QMetaType>

namespace MantidQt {
namespace CustomInterfaces {

struct MANTIDQT_ENGGDIFFRACTION_DLL GSASIIRefineFitPeaksOutputProperties {
  GSASIIRefineFitPeaksOutputProperties(const double _rwp, const double _sigma, const double _gamma,
                                       const Mantid::API::MatrixWorkspace_sptr &_fittedPeaksWS,
                                       const Mantid::API::ITableWorkspace_sptr &_latticeParamsWS,
                                       const RunLabel &_runLabel);

  GSASIIRefineFitPeaksOutputProperties() = default;

  double rwp;
  double sigma;
  double gamma;
  Mantid::API::MatrixWorkspace_sptr fittedPeaksWS;
  Mantid::API::ITableWorkspace_sptr latticeParamsWS;
  RunLabel runLabel;
};

MANTIDQT_ENGGDIFFRACTION_DLL bool operator==(const GSASIIRefineFitPeaksOutputProperties &lhs,
                                             const GSASIIRefineFitPeaksOutputProperties &rhs);

MANTIDQT_ENGGDIFFRACTION_DLL bool operator!=(const GSASIIRefineFitPeaksOutputProperties &lhs,
                                             const GSASIIRefineFitPeaksOutputProperties &rhs);

} // namespace CustomInterfaces
} // namespace MantidQt

Q_DECLARE_METATYPE(MantidQt::CustomInterfaces::GSASIIRefineFitPeaksOutputProperties)

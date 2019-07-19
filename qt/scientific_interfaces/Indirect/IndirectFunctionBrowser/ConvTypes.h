// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_INDIRECT_CONVTYPES_H_
#define MANTIDQT_INDIRECT_CONVTYPES_H_

#include "DllConfig.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidQtWidgets/Common/FunctionModel.h"
#include "ParameterEstimation.h"

#include <QMap>
#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
namespace ConvTypes {

enum class FitType { None, OneLorentzian, TwoLorentzians };

enum class ParamID {
  LOR1_AMPLITUDE,
  LOR1_PEAKCENTRE,
  LOR1_FWHM,
  LOR2_AMPLITUDE,
  LOR2_PEAKCENTRE,
  LOR2_FWHM,
  BG_A0,
  BG_A1
};

QStringList getFitTypes();
FitType fitTypeId(const QString &fitType);
QString paramName(ParamID id);
QStringList getParameterNames(FitType fitType);


} // namespace ConvTypes
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_INDIRECT_CONVTYPES_H_ */

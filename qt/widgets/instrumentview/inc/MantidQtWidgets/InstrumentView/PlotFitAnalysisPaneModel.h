// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_INSTRUMENTVIEW_PLOTFITANALYSISPANEMODEL_H_
#define MANTIDQT_INSTRUMENTVIEW_PLOTFITANALYSISPANEMODEL_H_

#include "MantidAPI/IFunction.h"

#include <map>
#include <string>
using namespace Mantid::API;
namespace MantidQt {
namespace MantidWidgets {

class PlotFitAnalysisPaneModel {

public:
  IFunction_sptr doFit(const std::string &wsName,
                       const std::pair<double, double> &range,
                       const IFunction_sptr func);
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTIDQT_INSTRUMENTVIEW_PLOTFITANALYSISPANEMODEL_H_ */

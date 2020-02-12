// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_FQFITTYPES_H_
#define MANTIDQTCUSTOMINTERFACES_FQFITTYPES_H_

#include <QStringList>
#include <unordered_map>

#include "IFQFitObserver.h"

enum class FitType {
  None,
  ChudleyElliot,
  HallRoss,
  FickDiffusion,
  TeixeiraWater,
  EISFDiffCylinder,
  EISFDiffSphere,
  EISFDiffSphereAlkyl,
};

extern std::unordered_map<DataType, QStringList> dataTypeFitTypeMap;

#endif

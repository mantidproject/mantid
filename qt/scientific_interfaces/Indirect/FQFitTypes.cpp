// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "FQFitTypes.h"

QStringList widthFits = QStringList(
    {"ChudleyElliot", "HallRoss", "FickDiffusion", "TeixeiraWater"});

QStringList eisfFits =
    QStringList({"EISFDiffCylinder", "EISFDiffSphere", "EISFDiffSphereAlkyl"});

std::unordered_map<DataType, QStringList> dataTypeFitTypeMap(
    {{DataType::WIDTH, widthFits}, {DataType::EISF, eisfFits}});
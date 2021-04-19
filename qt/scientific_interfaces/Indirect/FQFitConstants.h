// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <map>
#include <string>
#include <unordered_map>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

enum class DataType {
  WIDTH,
  EISF,
  ALL,
};

static const std::map<std::string, std::string> widthFits{
    {{"None", ""},
     {std::string("ChudleyElliot"), std::string("name=ChudleyElliot, Tau=1, L=1.5, constraints=(Tau>0, L>0)")},
     {std::string("HallRoss"), std::string("name=Hallross, Tau=1, L=0.2, constraints=(Tau>0, L>0)")},
     {std::string("FickDiffusion"), std::string("name=FickDiffusion, D=1, constraints=(D>0)")},
     {std::string("TeixeiraWater"), std::string("name=TeixeiraWater, Tau=1, L=1.5, constraints=(Tau>0, L>0)")}}};

static const std::map<std::string, std::string> EISFFits{
    {{"None", ""},
     {std::string("EISFDiffCylinder"),
      std::string("name=EISFDiffCylinder, A=1, R=1, L=2, constraints=(A>0, R>0, L>0)")},
     {std::string("EISFDiffSphere"), std::string("name=EISFDiffSphere, A=1, R=1, constraints=(A>0, R>0)")},
     {std::string("EISFDiffSphereAlkyl"), std::string("name=EISFDiffSphereAlkyl, A=1, Rmin=1, Rmax=2, "
                                                      "constraints=(A>0, Rmin>0, Rmax>0)")}}};

static const std::map<std::string, std::string> AllFits{
    {{"None", ""},
     {std::string("ChudleyElliot"), std::string("name=ChudleyElliot, Tau=1, L=1.5, constraints=(Tau>0, L>0)")},
     {std::string("HallRoss"), std::string("name=Hallross, Tau=1, L=0.2, constraints=(Tau>0, L>0)")},
     {std::string("FickDiffusion"), std::string("name=FickDiffusion, D=1, constraints=(D>0)")},
     {std::string("TeixeiraWater"), std::string("name=TeixeiraWater, Tau=1, L=1.5, constraints=(Tau>0, L>0)")},
     {std::string("EISFDiffCylinder"),
      std::string("name=EISFDiffCylinder, A=1, R=1, L=2, constraints=(A>0, R>0, L>0)")},
     {std::string("EISFDiffSphere"), std::string("name=EISFDiffSphere, A=1, R=1, constraints=(A>0, R>0)")},
     {std::string("EISFDiffSphereAlkyl"), std::string("name=EISFDiffSphereAlkyl, A=1, Rmin=1, Rmax=2, "
                                                      "constraints=(A>0, Rmin>0, Rmax>0)")}}};

static const std::unordered_map<DataType, std::map<std::string, std::string>> availableFits{
    {{DataType::WIDTH, widthFits}, {DataType::EISF, EISFFits}, {DataType::ALL, AllFits}}};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

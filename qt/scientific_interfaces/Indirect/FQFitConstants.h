// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <map>
#include <unordered_map>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

enum class DataType {
  WIDTH,
  EISF,
  ALL,
};

static const std::map<std::string, std::string>
    widthFits({{"ChudleyElliot", "name=ChudleyElliot"},
               {"HallRoss", "name=Hallross"},
               {"FickDiffusion", "name=FickDiffusion"},
               {"TeixeiraWater", "name=TeixeiraWater"}});

static const std::map<std::string, std::string>
    EISFFits({{"EISFDiffCylinder", "name=EISFDiffCylinder"},
              {"EISFDiffSphere", "name=EISFDiffSphere"},
              {"EISFDiffSphereAlkyl", "name=EISFDiffSphereAlkyl"}});

static const std::map<std::string, std::string>
    AllFits({{"ChudleyElliot", "name=ChudleyElliot"},
             {"HallRoss", "name=Hallross"},
             {"FickDiffusion", "name=FickDiffusion"},
             {"TeixeiraWater", "name=TeixeiraWater"},
             {"EISFDiffCylinder", "name=EISFDiffCylinder"},
             {"EISFDiffSphere", "name=EISFDiffSphere"},
             {"EISFDiffSphereAlkyl", "name=EISFDiffSphereAlkyl"}});

static const std::unordered_map<DataType, std::map<std::string, std::string>>
    availableFits({{DataType::WIDTH, widthFits},
                   {DataType::EISF, EISFFits},
                   {DataType::ALL, AllFits}});
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
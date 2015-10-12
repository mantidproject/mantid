#include "MantidMDAlgorithms/MDTransfAxisNames.h"
#include <boost/format.hpp>

namespace Mantid {
namespace MDAlgorithms {
using namespace Mantid::Kernel;

MDTransfAxisNames::MDTransfAxisNames() : m_DefaultDimID(CnvrtToMD::nDefaultID) {
  // this defines default dimension ID-s which are used to indentify dimensions
  // when using the target MD workspace later
  // for ModQ transformation:
  m_DefaultDimID[CnvrtToMD::ModQ_ID] = "|Q|";
  // for Q3D transformation
  m_DefaultDimID[CnvrtToMD::Q1_ID] = "Q1";
  m_DefaultDimID[CnvrtToMD::Q2_ID] = "Q2";
  m_DefaultDimID[CnvrtToMD::Q3_ID] = "Q3";
  m_DefaultDimID[CnvrtToMD::dE_ID] = "DeltaE";
}

//
std::vector<std::string>
MDTransfAxisNames::getDefaultDimIDQ3D(DeltaEMode::Type dEMode) const {
  std::vector<std::string> rez;
  if (dEMode == DeltaEMode::Elastic) {
    rez.resize(3);
  } else {

    if (dEMode == DeltaEMode::Direct || dEMode == DeltaEMode::Indirect) {
      rez.resize(4);
      rez[3] = m_DefaultDimID[CnvrtToMD::dE_ID];
    } else {
      throw(std::invalid_argument("Unknown dE mode provided"));
    }
  }
  rez[0] = m_DefaultDimID[CnvrtToMD::Q1_ID];
  rez[1] = m_DefaultDimID[CnvrtToMD::Q2_ID];
  rez[2] = m_DefaultDimID[CnvrtToMD::Q3_ID];
  return rez;
}

std::vector<std::string>
MDTransfAxisNames::getDefaultDimIDModQ(DeltaEMode::Type dEMode) const {
  std::vector<std::string> rez;

  if (dEMode == DeltaEMode::Elastic) {
    rez.resize(1);
  } else {
    if (dEMode == DeltaEMode::Direct || dEMode == DeltaEMode::Indirect) {
      rez.resize(2);
      rez[1] = m_DefaultDimID[CnvrtToMD::dE_ID];
    } else {
      throw(std::invalid_argument("Unknown dE mode provided"));
    }
  }
  rez[0] = m_DefaultDimID[CnvrtToMD::ModQ_ID];
  return rez;
}

std::string makeAxisName(const Kernel::V3D &Dir,
                         const std::vector<std::string> &QNames) {
  double eps(1.e-3);
  Kernel::V3D absDir(fabs(Dir.X()), fabs(Dir.Y()), fabs(Dir.Z()));
  std::string mainName;

  if ((absDir[0] >= absDir[1]) && (absDir[0] >= absDir[2])) {
    mainName = QNames[0];
  } else if (absDir[1] >= absDir[2]) {
    mainName = QNames[1];
  } else {
    mainName = QNames[2];
  }

  std::string name("["), separator = ",";
  for (size_t i = 0; i < 3; i++) {

    if (i == 2)
      separator = "]";
    if (absDir[i] < eps) {
      name += "0" + separator;
      continue;
    }
    if (Dir[i] < 0) {
      name += "-";
    }
    if (std::fabs(absDir[i] - 1) < eps) {
      name += mainName + separator;
      continue;
    }
    name += sprintfd(absDir[i], eps) + mainName + separator;
  }

  return name;
}
std::string DLLExport sprintfd(const double data, const double eps) {
  // truncate to eps decimal points
  float dist = float((int(data / eps + 0.5)) * eps);
  return boost::str(boost::format("%d") % dist);
}

} // endnamespace MDAlgorithms
} // endnamespace Mantid

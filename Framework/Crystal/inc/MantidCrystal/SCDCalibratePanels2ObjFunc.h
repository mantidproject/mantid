// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidCrystal/DllConfig.h"

namespace Mantid {
namespace Crystal {

/** SCDCalibratePanels2ObjFunc : TODO: DESCRIPTION
*/
class MANTID_CRYSTAL_DLL SCDCalibratePanels2ObjFunc : public API::ParamFunction,
                                                      public API::IFunction1D {
public:
    /// overwrite based class name
    std::string name() const override {return "SCDCalibratePanels2ObjFunc";}

    /// set category
    const std::string category() const override {return "General";}

private:
    
};

} // namespace Crystal
} // namespace Mantid

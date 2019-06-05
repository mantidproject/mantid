// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/SaveNISTDAT.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidKernel/CompositeValidator.h"

#include <fstream> // used to get ofstream

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveNISTDAT)

using namespace Kernel;
using namespace API;
using namespace Geometry;

void SaveNISTDAT::init() {
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add(
      boost::make_shared<WorkspaceUnitValidator>("MomentumTransfer"));
  wsValidator->add<HistogramValidator>();
  declareProperty(std::make_unique<WorkspaceProperty<>>(
      "InputWorkspace", "", Direction::Input, wsValidator));
  declareProperty(std::make_unique<FileProperty>("Filename", "",
                                                 FileProperty::Save, ".dat"),
                  "The filename of the output text file");
}

void SaveNISTDAT::exec() {
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  const std::string filename = getPropertyValue("Filename");

  // prepare to save to file
  std::ofstream out_File(filename.c_str());
  if (!out_File) {
    g_log.error("Failed to open file:" + filename);
    throw Exception::FileError("Failed to open file:", filename);
  }
  out_File << "Data columns Qx - Qy - I(Qx,Qy) - err(I)\r\n";
  out_File << "ASCII data\r\n";

  // Set up the progress reporting object
  Progress progress(this, 0.0, 1.0, 2);
  progress.report("Save I(Qx,Qy)");

  if (inputWS->axes() > 1 && inputWS->getAxis(1)->isNumeric()) {
    const Axis &axis = *inputWS->getAxis(1);
    for (size_t i = 0; i < axis.length() - 1; i++) {
      const double qy = (axis(i) + axis(i + 1)) / 2.0;
      const auto &XIn = inputWS->x(i);
      const auto &YIn = inputWS->y(i);
      const auto &EIn = inputWS->e(i);

      for (size_t j = 0; j < XIn.size() - 1; j++) {
        // Don't write out Q bins without data
        if (YIn[j] == 0 && EIn[j] == 0)
          continue;
        // Exclude NaNs
        if (YIn[j] == YIn[j]) {
          out_File << (XIn[j] + XIn[j + 1]) / 2.0;
          out_File << "  " << qy;
          out_File << "  " << YIn[j];
          out_File << "  " << EIn[j] << "\r\n";
        }
      }
    }
  }
  out_File.close();
  progress.report("Save I(Qx,Qy)");
}

} // namespace DataHandling
} // namespace Mantid

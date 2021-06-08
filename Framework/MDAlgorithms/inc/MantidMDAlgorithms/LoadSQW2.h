// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFileLoader.h"
#include "MantidDataObjects/MDEvent.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidKernel/BinaryStreamReader.h"
#include "MantidKernel/FileDescriptor.h"
#include <fstream>

namespace Mantid {

// Forward declarations
namespace API {
class ExperimentInfo;
}
namespace Geometry {
class OrientedLattice;
}

namespace MDAlgorithms {

class DLLExport LoadSQW2 : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"LoadNXSPE", "SaveNXSPE"}; }
  const std::string category() const override;
  const std::string summary() const override;
  int confidence(Kernel::FileDescriptor &descriptor) const override;

private:
  /// Local typedef for
  using SQWWorkspace = DataObjects::MDEventWorkspace<DataObjects::MDEvent<4>, 4>;

  void init() override;
  void exec() override;
  void cacheInputs();
  void initFileReader();
  int32_t readMainHeader();
  void throwIfUnsupportedFileType(int32_t sqwType);
  void createOutputWorkspace();
  void readAllSPEHeadersToWorkspace();
  std::shared_ptr<API::ExperimentInfo> readSingleSPEHeader();
  void cacheFrameTransforms(const Geometry::OrientedLattice &lattice);
  void skipDetectorSection();
  void readDataSection();
  void skipDataSectionMetadata();
  void readSQWDimensions();
  std::vector<int32_t> readProjection();
  std::vector<float> calculateDimLimitsFromData();
  Geometry::IMDDimension_sptr createQDimension(size_t index, float dimMin, float dimMax, size_t nbins,
                                               const Kernel::DblMatrix &bmat);
  Geometry::IMDDimension_sptr createEnDimension(float umin, float umax, size_t nbins);
  void setupBoxController();
  void setupFileBackend(const std::string &filebackPath);
  void readPixelDataIntoWorkspace();
  void splitAllBoxes();
  void warnIfMemoryInsufficient(int64_t npixtot);
  size_t addEventFromBuffer(const float *pixel);
  void toOutputFrame(coord_t *centers);
  void finalize();

  std::unique_ptr<std::ifstream> m_file;
  std::unique_ptr<Kernel::BinaryStreamReader> m_reader;
  std::shared_ptr<SQWWorkspace> m_outputWS;
  uint16_t m_nspe = 0;
  Kernel::DblMatrix m_uToRLU;
  std::string m_outputFrame;
};

} // namespace MDAlgorithms
} // namespace Mantid

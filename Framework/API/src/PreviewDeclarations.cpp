// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAPI/IPreview.h"
#include "MantidAPI/PreviewManager.h"

namespace Mantid {
namespace DataHandling {

// Spectroscopy
class SpecCountDiffractionCurve : public API::IPreview {
public:
  std::string name() const override { return "DiffractionCurve"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "Spectroscopy"; }
  std::string acquisition() const override { return "Count"; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::PLOT1D; }
};

class SpecTOFCountDiffractionCurve : public API::IPreview {
public:
  std::string name() const override { return "DiffractionCurve"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "Spectroscopy"; }
  std::string acquisition() const override { return "TOFCount"; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::PLOT1D; }
};

class SpecScanCountDiffractionCurveScan : public API::IPreview {
public:
  std::string name() const override { return "DiffractionCurveScan"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "Spectroscopy"; }
  std::string acquisition() const override { return "ScanCount"; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::PLOT2D; }
};

class SpecTOFCountPowderGrouping : public API::IPreview {
public:
  std::string name() const override { return "PowderGrouping"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "Spectroscopy"; }
  std::string acquisition() const override { return "TOFCount"; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::PLOT2D; }
};

class SpecAllRawDataInstrumentView : public API::IPreview {
public:
  std::string name() const override { return "InstrumentView"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "Spectroscopy"; }
  std::string acquisition() const override { return ""; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::IVIEW; }
};

// SANS
class SANSAllRawDataInstrumentView : public API::IPreview {
public:
  std::string name() const override { return "RawDataInstrumentView"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "SANS"; }
  std::string acquisition() const override { return ""; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::IVIEW; }
};

class SANSAllSolidAngleNormInstrumentView : public API::IPreview {
public:
  std::string name() const override { return "SolidAngleNormInstrumentView"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "SANS"; }
  std::string acquisition() const override { return ""; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::IVIEW; }
};

// Diffraction
class DiffractionCountDiffractionCurve : public API::IPreview {
public:
  std::string name() const override { return "DiffractionCurve"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "Diffraction"; }
  std::string acquisition() const override { return "Count"; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::PLOT1D; }
};

class DiffractionDetectorScanDiffractionCurve : public API::IPreview {
public:
  std::string name() const override { return "DiffractionCurve"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "Diffraction"; }
  std::string acquisition() const override { return "DetectorScan"; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::PLOT1D; }
};

// Reflectometry
class ReflectometryTOFCountRawDataInTOF : public API::IPreview {
public:
  std::string name() const override { return "RawDataInTOF"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "Reflectometry"; }
  std::string acquisition() const override { return "TOFCount"; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::PLOT2D; }
};

class ReflectometryTOFCountRawDataInWavelength : public API::IPreview {
public:
  std::string name() const override { return "RawDataInWavelength"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "Reflectometry"; }
  std::string acquisition() const override { return "TOFCount"; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::PLOT2D; }
};

// BackScattering
class BackScatteringDopplerSpectroscopyRawDataInChannels : public API::IPreview {
public:
  std::string name() const override { return "RawDataInChannels"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "BackScattering"; }
  std::string acquisition() const override { return "DopplerSpectroscopy"; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::PLOT2D; }
};

class BackScatteringDopplerDiffractionDiffractionCurve : public API::IPreview {
public:
  std::string name() const override { return "DiffractionCurve"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "BackScattering"; }
  std::string acquisition() const override { return "DopplerDiffraction"; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::PLOT1D; }
};

class BackScatteringBatsSpectroscopyRawDataInChannels : public API::IPreview {
public:
  std::string name() const override { return "RawDataInChannels"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "BackScattering"; }
  std::string acquisition() const override { return "BatsSpectroscopy"; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::PLOT2D; }
};

class BackScatteringBatsDiffractionRawDataInChannels : public API::IPreview {
public:
  std::string name() const override { return "RawDataInChannels"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "BackScattering"; }
  std::string acquisition() const override { return "BatsDiffraction"; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::PLOT2D; }
};

// Powder diffraction
class PowderDiffractionCountDiffractionCurve : public API::IPreview {
public:
  std::string name() const override { return "DiffractionCurve"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "PowderDiffraction"; }
  std::string acquisition() const override { return "Count"; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::PLOT1D; }
};

class PowderDiffractionTOFCountRawDataInChannels : public API::IPreview {
public:
  std::string name() const override { return "RawDataInChannels"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "PowderDiffraction"; }
  std::string acquisition() const override { return "TOFCount"; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::PLOT2D; }
};

// Previews actual declaration
DECLARE_PREVIEW(SpecCountDiffractionCurve)
DECLARE_PREVIEW(SpecTOFCountDiffractionCurve)
DECLARE_PREVIEW(SpecScanCountDiffractionCurveScan)
DECLARE_PREVIEW(SpecTOFCountPowderGrouping)
DECLARE_PREVIEW(SpecAllRawDataInstrumentView)

DECLARE_PREVIEW(SANSAllRawDataInstrumentView)
DECLARE_PREVIEW(SANSAllSolidAngleNormInstrumentView)

DECLARE_PREVIEW(DiffractionCountDiffractionCurve)
DECLARE_PREVIEW(DiffractionDetectorScanDiffractionCurve)

DECLARE_PREVIEW(ReflectometryTOFCountRawDataInTOF)
DECLARE_PREVIEW(ReflectometryTOFCountRawDataInWavelength)

DECLARE_PREVIEW(BackScatteringDopplerSpectroscopyRawDataInChannels)
DECLARE_PREVIEW(BackScatteringDopplerDiffractionDiffractionCurve)
DECLARE_PREVIEW(BackScatteringBatsSpectroscopyRawDataInChannels)
DECLARE_PREVIEW(BackScatteringBatsDiffractionRawDataInChannels)

DECLARE_PREVIEW(PowderDiffractionCountDiffractionCurve)
DECLARE_PREVIEW(PowderDiffractionTOFCountRawDataInChannels)
} // namespace DataHandling
} // namespace Mantid

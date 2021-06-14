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
  std::string geometry() const override { return ""; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::PLOT1D; }
};

class SpecTOFCountDiffractionCurve : public API::IPreview {
public:
  std::string name() const override { return "DiffractionCurve"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "Spectroscopy"; }
  std::string acquisition() const override { return "TOFCount"; }
  std::string geometry() const override { return ""; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::PLOT1D; }
};

class SpecScanCountDiffractionCurveScan : public API::IPreview {
public:
  std::string name() const override { return "DiffractionCurveScan"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "Spectroscopy"; }
  std::string acquisition() const override { return "ScanCount"; }
  std::string geometry() const override { return ""; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::PLOT2D; }
};

class SpecTOFCountPowderGrouping : public API::IPreview {
public:
  std::string name() const override { return "PowderGrouping"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "Spectroscopy"; }
  std::string acquisition() const override { return "TOFCount"; }
  std::string geometry() const override { return ""; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::PLOT2D; }
};

class SpecAllRawDataInstrumentView : public API::IPreview {
public:
  std::string name() const override { return "InstrumentView"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "Spectroscopy"; }
  std::string acquisition() const override { return ""; }
  std::string geometry() const override { return ""; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::IVIEW; }
};

// SANS
class SANSAllRawDataInstrumentView : public API::IPreview {
public:
  std::string name() const override { return "RawDataInstrumentView"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "SANS"; }
  std::string acquisition() const override { return ""; }
  std::string geometry() const override { return ""; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::IVIEW; }
};

class SANSAllSolidAngleNormInstrumentView : public API::IPreview {
public:
  std::string name() const override { return "SolidAngleNormInstrumentView"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "SANS"; }
  std::string acquisition() const override { return ""; }
  std::string geometry() const override { return ""; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::IVIEW; }
};

// Diffraction
class DiffractionCountDiffractionCurve : public API::IPreview {
public:
  std::string name() const override { return "DiffractionCurve"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "Diffraction"; }
  std::string acquisition() const override { return "Count"; }
  std::string geometry() const override { return ""; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::PLOT1D; }
};

class DiffractionDetectorScanDiffractionCurve : public API::IPreview {
public:
  std::string name() const override { return "DiffractionCurve"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "Diffraction"; }
  std::string acquisition() const override { return "DetectorScan"; }
  std::string geometry() const override { return ""; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::PLOT1D; }
};

// REFL
class REFLRawDataPlot1D : public API::IPreview {
public:
  std::string name() const override { return "Plot1DSpectrum | RawData"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "REFL"; }
  std::string acquisition() const override { return "TOF"; }
  std::string geometry() const override { return "1D"; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::PLOT1D; }
};

class REFLRawDataSliceViewer : public API::IPreview {
public:
  std::string name() const override { return "SliceViewer | RawData"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "REFL"; }
  std::string acquisition() const override { return "TOF"; }
  std::string geometry() const override { return "1D"; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::SVIEW; }
};

class REFLRawDataPlot2D : public API::IPreview {
public:
  std::string name() const override { return "Plot2D | RawData"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "REFL"; }
  std::string acquisition() const override { return "TOF"; }
  std::string geometry() const override { return "1D"; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::PLOT2D; }
};

// BACK
// TODO

// TODO SCAN

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

DECLARE_PREVIEW(PDIFFMonoRawDataPlot1D)
DECLARE_PREVIEW(PDIFFTOFRawDataSliceViewer)
DECLARE_PREVIEW(PDIFFTOFRawDataPlot1D)
DECLARE_PREVIEW(PDIFFTOFRawDataPlot2D)

DECLARE_PREVIEW(REFLRawDataSliceViewer)
DECLARE_PREVIEW(REFLRawDataPlot1D)
DECLARE_PREVIEW(REFLRawDataPlot2D)

} // namespace DataHandling
} // namespace Mantid

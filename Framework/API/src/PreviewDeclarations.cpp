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

// SANS
class SANSMonoRawDataInstrumentView : public API::IPreview {
public:
  std::string name() const override { return "IView | RawData"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "SANS"; }
  std::string acquisition() const override { return "Mono"; }
  std::string geometry() const override { return "2D"; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::IVIEW; }
};

class SANSTOFRawDataPlot2D : public API::IPreview {
public:
  std::string name() const override { return "Plot2D | RawData"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "SANS"; }
  std::string acquisition() const override { return "TOF"; }
  std::string geometry() const override { return "2D"; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::PLOT2D; }
};

class SANSTOFRawDataPlot1D : public API::IPreview {
public:
  std::string name() const override { return "Plot1DSpectrum | RawData"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "SANS"; }
  std::string acquisition() const override { return "TOF"; }
  std::string geometry() const override { return "2D"; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::PLOT1D; }
};

class SANSTOFRawDataInstrumentView : public API::IPreview {
public:
  std::string name() const override { return "IView | RawData"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "SANS"; }
  std::string acquisition() const override { return "TOF"; }
  std::string geometry() const override { return "2D"; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::IVIEW; }
};

class SANSTOFRawDataSliceViewer : public API::IPreview {
public:
  std::string name() const override { return "SliceViewer | RawData"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "SANS"; }
  std::string acquisition() const override { return "TOF"; }
  std::string geometry() const override { return "2D"; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::SVIEW; }
};

/*class SANSOScanRawDataInstrumentView : public API::IPreview {
public:
  std::string name() const override { return "IView | RawData"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "SANS"; }
  std::string acquisition() const override { return "OScan"; }
  std::string geometry() const override { return "2D"; }
  API::IPreview::PreviewType type() const override {
    return API::IPreview::PreviewType::IVIEW;
  }
};*/

// PDIFF
class PDIFFTOFRawDataPlot1D : public API::IPreview {
public:
  std::string name() const override { return "Plot1DSpectrum | RawData"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "PDIFF"; }
  std::string acquisition() const override { return "TOF"; }
  std::string geometry() const override { return "1D"; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::PLOT1D; }
};

class PDIFFTOFRawDataPlot2D : public API::IPreview {
public:
  std::string name() const override { return "Plot2D | RawData"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "PDIFF"; }
  std::string acquisition() const override { return "TOF"; }
  std::string geometry() const override { return "1D"; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::PLOT2D; }
};

class PDIFFTOFRawDataSliceViewer : public API::IPreview {
public:
  std::string name() const override { return "SliceViewer | RawData"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "PDIFF"; }
  std::string acquisition() const override { return "TOF"; }
  std::string geometry() const override { return "1D"; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::SVIEW; }
};

class PDIFFMonoRawDataPlot1D : public API::IPreview {
public:
  std::string name() const override { return "Plot1DBin | RawData"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "PDIFF"; }
  std::string acquisition() const override { return "Mono"; }
  std::string geometry() const override { return "1D"; }
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

// DIFF
// TODO

// BACK
// TODO

// SPEC
// TODO
class SpecRawDataMonoIview : public API::IPreview {
public:
  std::string name() const override { return "IView | RawData"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "SPEC"; }
  std::string acquisition() const override { return "Mono"; }
  std::string geometry() const override { return "2D"; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::IVIEW; }
};

class SpecRawDataTOFPlot1D : public API::IPreview {
public:
  std::string name() const override { return "Plot1DSpectrum | RawData"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "SPEC"; }
  std::string acquisition() const override { return "TOF"; }
  std::string geometry() const override { return "2D"; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::PLOT1D; }
};

class SpecRawDataTOFPlot2D : public API::IPreview {
public:
  std::string name() const override { return "Plot2D | RawData"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "SPEC"; }
  std::string acquisition() const override { return "TOF"; }
  std::string geometry() const override { return "2D"; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::PLOT2D; }
};

class SpecRawDataTOFInstrumentView : public API::IPreview {
public:
  std::string name() const override { return "IView | RawData"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "SPEC"; }
  std::string acquisition() const override { return "TOF"; }
  std::string geometry() const override { return "2D"; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::IVIEW; }
};

class SpecRawDataTOFSliceViewer : public API::IPreview {
public:
  std::string name() const override { return "SliceViewer | RawData"; }
  std::string facility() const override { return "ILL"; }
  std::string technique() const override { return "SPEC"; }
  std::string acquisition() const override { return "TOF"; }
  std::string geometry() const override { return "2D"; }
  API::IPreview::PreviewType type() const override { return API::IPreview::PreviewType::SVIEW; }
};

// TODO SCAN

// Previews actual declaration
DECLARE_PREVIEW(SANSMonoRawDataInstrumentView)
DECLARE_PREVIEW(SANSTOFRawDataInstrumentView)
DECLARE_PREVIEW(SANSTOFRawDataSliceViewer)
DECLARE_PREVIEW(SANSTOFRawDataPlot1D)
DECLARE_PREVIEW(SANSTOFRawDataPlot2D)

DECLARE_PREVIEW(PDIFFMonoRawDataPlot1D)
DECLARE_PREVIEW(PDIFFTOFRawDataSliceViewer)
DECLARE_PREVIEW(PDIFFTOFRawDataPlot1D)
DECLARE_PREVIEW(PDIFFTOFRawDataPlot2D)

DECLARE_PREVIEW(REFLRawDataSliceViewer)
DECLARE_PREVIEW(REFLRawDataPlot1D)
DECLARE_PREVIEW(REFLRawDataPlot2D)

DECLARE_PREVIEW(SpecRawDataMonoIview)
DECLARE_PREVIEW(SpecRawDataTOFInstrumentView)
DECLARE_PREVIEW(SpecRawDataTOFSliceViewer)
DECLARE_PREVIEW(SpecRawDataTOFPlot1D)
DECLARE_PREVIEW(SpecRawDataTOFPlot2D)
} // namespace DataHandling
} // namespace Mantid

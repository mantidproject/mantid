#ifndef MANTID_CUSTOMINTERFACES_IMAGINGFORMATSCONVERTVIEWMOCK_H
#define MANTID_CUSTOMINTERFACES_IMAGINGFORMATSCONVERTVIEWMOCK_H

#include "MantidQtCustomInterfaces/Tomography/ITomographyIfaceView.h"

#include <gmock/gmock.h>

// This is a simple mock for the tomo interface view when using SCARF.
class ImagingFormatsConvertViewMock
    : public MantidQt::CustomInterfaces::IImagingFormatsConvertView {
public:
  // void userWarning(const std::string &warn, const std::string &description)
  MOCK_METHOD2(userWarning,
               void(const std::string &warn, const std::string &description));

  // void userError(const std::string &err, const std::string &description)
  MOCK_METHOD2(userError,
               void(const std::string &err, const std::string &description));

  // void setFormats(const std::vector<std::string> &fmts, const
  // std::vector<bool> &enable)
  MOCK_METHOD2(setFormats, void(const std::vector<std::string> &fmts,
                                const std::vector<bool> &enable));

  // std::string inputPath() const
  MOCK_CONST_METHOD0(inputPath, std::string());

  // std::string inputFormatName() const
  MOCK_CONST_METHOD0(inputFormatName, std::string());

  // std::string outputPath() const
  MOCK_CONST_METHOD0(outputPath, std::string());

  // std::string outputFormatName() const
  MOCK_CONST_METHOD0(outputFormatName, std::string());

  // bool compressHint() const
  MOCK_CONST_METHOD0(compressHint, bool());

  // void saveSettings() const
  MOCK_CONST_METHOD0(saveSettings, void());
};

#endif // MANTID_CUSTOMINTERFACES_IMAGINGFORMATSCONVERTVIEWMOCK_H

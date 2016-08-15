#ifndef MANTID_CUSTOMINTERFACES_IMGGFORMATSCONVERTVIEWMOCK_H
#define MANTID_CUSTOMINTERFACES_IMGGFORMATSCONVERTVIEWMOCK_H

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtCustomInterfaces/Tomography/IImggFormatsConvertView.h"

#include <gmock/gmock.h>

GCC_DIAG_OFF_SUGGEST_OVERRIDE

// This is a simple mock for the tomo interface view when using SCARF.
class ImggFormatsConvertViewMock
    : public MantidQt::CustomInterfaces::IImggFormatsConvertView {
public:
  // void userWarning(const std::string &warn, const std::string &description)
  MOCK_METHOD2(userWarning,
               void(const std::string &warn, const std::string &description));

  // void userError(const std::string &err, const std::string &description)
  MOCK_METHOD2(userError,
               void(const std::string &err, const std::string &description));

  // void setFormats(const std::vector<std::string> &fmts,
  // std::vector<bool> &enableLoad, std::vector<bool> &enableSave)
  MOCK_METHOD3(setFormats, void(const std::vector<std::string> &fmts,
                                const std::vector<bool> &enableLoad,
                                const std::vector<bool> &enableSave));

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

  // void convert(const std::string &inputName, const std::string
  // &inputFormat, const std::string &outputName, const std::string
  // &outputFormat) const
  MOCK_CONST_METHOD4(convert, void(const std::string &inputName,
                                   const std::string &inputFormat,
                                   const std::string &outputName,
                                   const std::string &outputFormat));

  // void writeImg(MatrixWorkspace_sptr inWks, const std::string
  // &outputName, const std::string &outFormat) const

  MOCK_CONST_METHOD3(writeImg, void(Mantid::API::MatrixWorkspace_sptr inWks,
                                    const std::string &outputName,
                                    const std::string &outFormat));

  // MatrixWorkspace_sptr loadImg(const std::string &inputName, const
  // std::string &inputFormat) const
  MOCK_CONST_METHOD2(
      loadImg, Mantid::API::MatrixWorkspace_sptr(const std::string &outputName,
                                                 const std::string &outFormat));

  // size_t maxSearchDepth() const
  MOCK_CONST_METHOD0(maxSearchDepth, size_t());

  // void saveSettings() const
  MOCK_CONST_METHOD0(saveSettings, void());
};

GCC_DIAG_ON_SUGGEST_OVERRIDE
#endif // MANTID_CUSTOMINTERFACES_IMGGFORMATSCONVERTVIEWMOCK_H

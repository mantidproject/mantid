#ifndef MANTID_CUSTOMINTERFACES_IMAGINGFORMATSCONVERTVIEWMOCK_H
#define MANTID_CUSTOMINTERFACES_IMAGINGFORMATSCONVERTVIEWMOCK_H

#include "MantidQtCustomInterfaces/Tomography/ITomographyIfaceView.h"

#include <gmock/gmock.h>

// This is a simple mock for the tomo interface view when using SCARF.
class ImagingFormatsConvertViewMock
    : public MantidQt::CustomInterfaces::IImagingFormatsConvertView {

  // void userWarning(const std::string &warn, const std::string &description)
  // {}
  MOCK_METHOD2(userWarning,
               void(const std::string &warn, const std::string &description));

  // void userError(const std::string &err, const std::string &description) {}
  MOCK_METHOD2(userError,
               void(const std::string &err, const std::string &description));

  // void saveSettings() const {}
  MOCK_CONST_METHOD0(saveSettings, void());

  };

#endif // MANTID_CUSTOMINTERFACES_IMAGINGFORMATSCONVERTVIEWMOCK_H

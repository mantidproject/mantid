#ifndef MANTID_CUSTOMINTERFACES_IMAGECORVIEWMOCK_H
#define MANTID_CUSTOMINTERFACES_IMAGECORVIEWMOCK_H

#include "MantidQtCustomInterfaces/Tomography/ITomographyIfaceView.h"

#include <gmock/gmock.h>

class MockImageCoRView : public MantidQt::CustomInterfaces::IImageCoRView {
public:
  // void initParams(ImageStackPreParams &params)
  MOCK_METHOD1(initParams,
               void(MantidQt::CustomInterfaces::ImageStackPreParams &));

  // ImageStackPreParams userSelection() const;
  MOCK_CONST_METHOD0(userSelection,
                     MantidQt::CustomInterfaces::ImageStackPreParams());

  // virtual std::string stackPath() const;
  MOCK_CONST_METHOD0(stackPath, std::string());

  // void showStack(const std::string &path);
  MOCK_METHOD1(showStack, void(const std::string &));

  // void showStack(const Mantid::API::WorkspaceGroup_sptr &ws);
  MOCK_METHOD1(showStack, void(const Mantid::API::WorkspaceGroup_sptr &));

  // void userWarning(const std::string &warn, const std::string &description)
  MOCK_METHOD2(userWarning,
               void(const std::string &warn, const std::string &description));

  // void userError(const std::string &err, const std::string &description)
  MOCK_METHOD2(userError,
               void(const std::string &err, const std::string &description));

  // virtual std::string askImgOrStackPath() = 0;
  MOCK_METHOD0(askImgOrStackPath, std::string());

  // void saveSettings() const {}
  MOCK_CONST_METHOD0(saveSettings, void());
};

#endif // MANTID_CUSTOMINTERFACES_IMAGECORVIEWMOCK_H

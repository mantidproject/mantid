#ifndef MANTID_CUSTOMINTERFACES_IMAGEROIVIEWMOCK_H
#define MANTID_CUSTOMINTERFACES_IMAGEROIVIEWMOCK_H

#include "MantidQtCustomInterfaces/Tomography/ITomographyIfaceView.h"

#include <gmock/gmock.h>

class MockImageROIView : public MantidQt::CustomInterfaces::IImageROIView {
public:
  // void initParams(ImageStackPreParams &params)
  MOCK_METHOD1(setParams,
               void(MantidQt::CustomInterfaces::ImageStackPreParams &));

  // ImageStackPreParams userSelection() const;
  MOCK_CONST_METHOD0(userSelection,
                     MantidQt::CustomInterfaces::ImageStackPreParams());

  // SelectionState selectionState() const;
  MOCK_CONST_METHOD0(selectionState, SelectionState());

  // void changeSelectionState(const SelectionState state);
  MOCK_METHOD1(changeSelectionState, void(const IImageROIView::SelectionState&));

  // void showStack(const std::string &path);
  MOCK_METHOD1(showStack, void(const std::string &));

  // void showStack(const Mantid::API::WorkspaceGroup_sptr &ws);
  MOCK_METHOD1(showStack, void(Mantid::API::WorkspaceGroup_sptr &));

  // const Mantid::API::WorkspaceGroup_sptr stack() const;
  MOCK_CONST_METHOD0(stack, const Mantid::API::WorkspaceGroup_sptr());

  // void showProjection(const Mantid::API::WorkspaceGroup_sptr &wsg, size_t idx);
  MOCK_METHOD2(showProjection, void(const Mantid::API::WorkspaceGroup_sptr &wsg, size_t idx));

  // void userWarning(const std::string &warn, const std::string &description)
  MOCK_METHOD2(userWarning,
               void(const std::string &warn, const std::string &description));

  // void userError(const std::string &err, const std::string &description)
  MOCK_METHOD2(userError,
               void(const std::string &err, const std::string &description));

  // size_t currentImgIndex() const;
  MOCK_CONST_METHOD0(currentImgIndex, size_t());

  // void updateImgWithIndex(size_t idx)
  MOCK_METHOD1(updateImgWithIndex, void(size_t));

  // std::string askImgOrStackPath();
  MOCK_METHOD0(askImgOrStackPath, std::string());

  // void saveSettings() const {}
  MOCK_CONST_METHOD0(saveSettings, void());

  // void resetCoR()
  MOCK_METHOD0(resetCoR, void());

  // void resetROI()
  MOCK_METHOD0(resetROI, void());

  // void resetNormArea()
  MOCK_METHOD0(resetNormArea, void());
};

#endif // MANTID_CUSTOMINTERFACES_IMAGEROIVIEWMOCK_H

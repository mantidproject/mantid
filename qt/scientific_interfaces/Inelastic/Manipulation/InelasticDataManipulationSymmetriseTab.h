// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "InelasticDataManipulationTab.h"

#include "InelasticDataManipulationSymmetriseTabModel.h"
#include "InelasticDataManipulationSymmetriseTabView.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/System.h"
#include "ui_InelasticDataManipulationSymmetriseTab.h"

// Suppress a warning coming out of code that isn't ours
#if defined(__INTEL_COMPILER)
#pragma warning disable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#include "MantidQtWidgets/Common/QtPropertyBrowser/QtCheckBoxFactory"
#if defined(__INTEL_COMPILER)
#pragma warning enable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
#endif

namespace MantidQt {
namespace CustomInterfaces {

class ISymmetrisePresenter {
public:
  virtual void handleReflectTypeChanged(int value) = 0;
  virtual void handleDoubleValueChanged(std::string const &propname, double value) = 0;
  virtual void handleDataReady(std::string const &dataName) = 0;
  virtual void handlePreviewClicked() = 0;
  virtual void handleRunClicked() = 0;
  virtual void handleSaveClicked() = 0;
};

/** InelasticDataManipulationSymmetriseTab

  @author Dan Nixon
  @date 23/07/2014
*/
class MANTIDQT_INELASTIC_DLL InelasticDataManipulationSymmetriseTab : public InelasticDataManipulationTab,
                                                                      public ISymmetrisePresenter {
public:
  InelasticDataManipulationSymmetriseTab(QWidget *parent, ISymmetriseView *view);
  ~InelasticDataManipulationSymmetriseTab() override;

  void setup() override;
  void run() override;
  bool validate() override;

  void handleReflectTypeChanged(int value) override;
  void handleDoubleValueChanged(std::string const &propname, double value) override;
  void handleDataReady(std::string const &dataName) override;
  void handlePreviewClicked() override;
  void handleRunClicked() override;
  void handleSaveClicked() override;

  void setIsPreview(bool preview);

protected:
  void runComplete(bool error) override;

private:
  void setFileExtensionsByName(bool filter) override;

  // wether batch algorunner is running preview or run buttons
  bool m_isPreview;
  Mantid::API::AnalysisDataServiceImpl &m_adsInstance;
  ISymmetriseView *m_view;
  std::unique_ptr<InelasticDataManipulationSymmetriseTabModel> m_model;
};
} // namespace CustomInterfaces
} // namespace MantidQt

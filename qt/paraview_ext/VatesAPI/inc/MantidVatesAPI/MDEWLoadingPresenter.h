// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_VATES_MDEW_LOADING_PRESENTER
#define MANTID_VATES_MDEW_LOADING_PRESENTER

#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"
#include "MantidVatesAPI/MDLoadingPresenter.h"
#include "MantidVatesAPI/MetaDataExtractorUtils.h"
#include "MantidVatesAPI/MetadataJsonManager.h"
#include "MantidVatesAPI/VatesConfigurations.h"

#include <boost/scoped_ptr.hpp>

namespace Mantid {
namespace VATES {

/**
@class MDEWLoadingPresenter
Abstract presenter encapsulating common operations used by all MDEW type
loading. Reduces template bloat.
@author Owen Arnold, Tessella plc
@date 16/08/2011
*/
class MDLoadingView;
class DLLExport MDEWLoadingPresenter : public MDLoadingPresenter {
public:
  MDEWLoadingPresenter(std::unique_ptr<MDLoadingView> view);
  const std::string &getGeometryXML() const override;
  bool hasTDimensionAvailable() const override;
  std::vector<double> getTimeStepValues() const override;
  std::string getTimeStepLabel() const override;
  void setAxisLabels(vtkDataSet *visualDataSet) override;
  ~MDEWLoadingPresenter() override;
  const std::string &getInstrument() override;

protected:
  /*---------------------------------------------------------------------------
  Common/shared operations and members for all MDEW file-type loading.
  ---------------------------------------------------------------------------*/
  std::unique_ptr<MDLoadingView> m_view;
  Mantid::Geometry::MDGeometryBuilderXML<Mantid::Geometry::NoDimensionPolicy>
      xmlBuilder;

  Mantid::Geometry::IMDDimension_sptr tDimension;

  std::vector<std::string> axisLabels;
  virtual void appendMetadata(vtkDataSet *visualDataSet,
                              const std::string &wsName);
  virtual void extractMetadata(const Mantid::API::IMDEventWorkspace &eventWs);
  virtual bool
  canLoadFileBasedOnExtension(const std::string &filename,
                              const std::string &expectedExtension) const;
  virtual bool shouldLoad();
  bool m_isSetup;
  double m_time;
  size_t m_recursionDepth;
  bool m_loadInMemory;
  bool m_firstLoad;

  boost::scoped_ptr<MetadataJsonManager> m_metadataJsonManager;
  boost::scoped_ptr<MetaDataExtractorUtils> m_metaDataExtractor;
  boost::scoped_ptr<VatesConfigurations> m_vatesConfigurations;
};
} // namespace VATES
} // namespace Mantid

#endif

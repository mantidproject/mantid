// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_VATES_MDHW_LOADING_PRESENTER
#define MANTID_VATES_MDHW_LOADING_PRESENTER

#include "MantidAPI/IMDHistoWorkspace_fwd.h"
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
@class MDHWLoadingPresenter
Abstract presenter encapsulating common operations used by all MDHW type
loading. Reduces template bloat.
@author Owen Arnold, Tessella plc
@date 16/08/2011
*/
class MDLoadingView;
class DLLExport MDHWLoadingPresenter : public MDLoadingPresenter {
public:
  MDHWLoadingPresenter(std::unique_ptr<MDLoadingView> view);
  const std::string &getGeometryXML() const override;
  bool hasTDimensionAvailable() const override;
  std::vector<double> getTimeStepValues() const override;
  std::string getTimeStepLabel() const override;
  void setAxisLabels(vtkDataSet *visualDataSet) override;
  ~MDHWLoadingPresenter() override;
  const std::string &getInstrument() override;

  /// Transpose a workspace to push integrated dimensions to the last
  static void
  transposeWs(Mantid::API::IMDHistoWorkspace_sptr &inHistoWs,
              Mantid::API::IMDHistoWorkspace_sptr &outCachedHistoWs);

protected:
  /*---------------------------------------------------------------------------
  Common/shared operations and members for all MDHW file-type loading.
  ---------------------------------------------------------------------------*/
  std::unique_ptr<MDLoadingView> m_view;

  Mantid::Geometry::MDGeometryBuilderXML<Mantid::Geometry::NoDimensionPolicy>
      xmlBuilder;

  Mantid::Geometry::IMDDimension_sptr tDimension;
  std::vector<std::string> axisLabels;
  virtual void appendMetadata(vtkDataSet *visualDataSet,
                              const std::string &wsName);
  virtual void extractMetadata(const Mantid::API::IMDHistoWorkspace &histoWs);
  virtual bool
  canLoadFileBasedOnExtension(const std::string &filename,
                              const std::string &expectedExtension) const;
  virtual bool shouldLoad();
  bool m_isSetup;
  double m_time;
  bool m_loadInMemory;
  bool m_firstLoad;

  boost::scoped_ptr<MetadataJsonManager> m_metadataJsonManager;
  boost::scoped_ptr<MetaDataExtractorUtils> m_metaDataExtractor;
  boost::scoped_ptr<VatesConfigurations> m_vatesConfigurations;
};
} // namespace VATES
} // namespace Mantid

#endif

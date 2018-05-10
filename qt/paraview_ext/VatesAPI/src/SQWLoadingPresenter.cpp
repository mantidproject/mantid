#include "MantidVatesAPI/SQWLoadingPresenter.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidVatesAPI/Common.h"
#include "MantidVatesAPI/MDLoadingView.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include <boost/regex.hpp>

namespace Mantid {
namespace VATES {

/*
    Constructor
    @param view : MVP view
    @param filename : name of file to load
    @throw invalid_argument if file name is empty
    @throw invalid_arument if view is null
    @throw logic_error if cannot use the reader-presenter for this filetype.
    */
SQWLoadingPresenter::SQWLoadingPresenter(std::unique_ptr<MDLoadingView> view,
                                         const std::string &filename)
    : MDEWLoadingPresenter(std::move(view)), m_filename(filename),
      m_wsTypeName("") {
  if (this->m_filename.empty()) {
    throw std::invalid_argument("File name is an empty string.");
  }
  if (nullptr == this->m_view) {
    throw std::invalid_argument("View is NULL.");
  }
}

/*
Indicates whether this presenter is capable of handling the type of file that
is attempted to be loaded.
@return false if the file cannot be read.
*/
bool SQWLoadingPresenter::canReadFile() const {
  boost::regex expression(
      ".*sqw$",
      boost::regex_constants::icase); // check that the file ends with sqw.
  return boost::regex_match(this->m_filename, expression);
}

/*
Executes the underlying algorithm to create the MVP model.
@param factory : visualisation factory to use.
@param loadingProgressUpdate : Handler for GUI updates while algorithm
progresses.
@param drawingProgressUpdate : Handler for GUI updates while
vtkDataSetFactory::create occurs.
*/
vtkSmartPointer<vtkDataSet>
SQWLoadingPresenter::execute(vtkDataSetFactory *factory,
                             ProgressAction &loadingProgressUpdate,
                             ProgressAction &drawingProgressUpdate) {
  using namespace Mantid::API;
  using namespace Mantid::Geometry;

  if (this->shouldLoad()) {
    Poco::NObserver<ProgressAction,
                    Mantid::API::Algorithm::ProgressNotification>
        observer(loadingProgressUpdate, &ProgressAction::handler);
    AnalysisDataService::Instance().remove("MD_EVENT_WS_ID");

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("LoadSQW");

    alg->initialize();
    alg->setPropertyValue("Filename", this->m_filename);
    alg->setPropertyValue("OutputWorkspace", "MD_EVENT_WS_ID");
    // Default is not to load into memory and when this is the case, generate a
    // nxs backend for output.
    if (!this->m_view->getLoadInMemory()) {
      size_t pos = this->m_filename.find('.');
      std::string backEndFile = this->m_filename.substr(0, pos) + ".nxs";
      alg->setPropertyValue("OutputFilename", backEndFile);
    }
    alg->addObserver(observer);
    alg->execute();
    alg->removeObserver(observer);
  }

  Workspace_sptr result =
      AnalysisDataService::Instance().retrieve("MD_EVENT_WS_ID");
  Mantid::API::IMDEventWorkspace_sptr eventWs =
      boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(result);

  factory->setRecursionDepth(this->m_view->getRecursionDepth());

  auto visualDataSet = factory->oneStepCreate(eventWs, drawingProgressUpdate);

  this->appendMetadata(visualDataSet, eventWs->getName());

  return visualDataSet;
}

/*
Extract the geometry and function information

This implementation is an override of the base-class method, which deals with
the more common event based route. However the SQW files will provide complete
dimensions with ranges already set. Less work needs to be done here than for
event workspaces where the extents of each dimension need to be individually
extracted.

@param eventWs : event workspace to get the information from.
*/
void SQWLoadingPresenter::extractMetadata(
    const Mantid::API::IMDEventWorkspace &eventWs) {
  using namespace Mantid::Geometry;
  MDGeometryBuilderXML<NoDimensionPolicy> refresh;
  this->xmlBuilder = refresh; // Reassign.
  std::vector<IMDDimension_sptr> dimensions;
  size_t nDimensions = eventWs.getNumDims();
  for (size_t d = 0; d < nDimensions; d++) {
    IMDDimension_const_sptr inDim = eventWs.getDimension(d);
    axisLabels.push_back(makeAxisTitle(*inDim));
    // Copy the dimension, but set the ID and name to be the same. This is an
    // assumption in bintohistoworkspace.
    dimensions.push_back(boost::make_shared<MDHistoDimension>(
        inDim->getName(), inDim->getName(), inDim->getMDFrame(),
        inDim->getMinimum(), inDim->getMaximum(), size_t{10}));
  }

  // Configuring the geometry xml builder allows the object panel associated
  // with this reader to later
  // determine how to display all geometry related properties.
  if (nDimensions > 0) {
    this->xmlBuilder.addXDimension(dimensions[0]);
  }
  if (nDimensions > 1) {
    this->xmlBuilder.addYDimension(dimensions[1]);
  }
  if (nDimensions > 2) {
    this->xmlBuilder.addZDimension(dimensions[2]);
  }
  if (nDimensions > 3) {
    this->tDimension = dimensions[3];
    this->xmlBuilder.addTDimension(this->tDimension);
  }
  this->m_isSetup = true;
}

/**
 Executes any meta-data loading required.
*/
void SQWLoadingPresenter::executeLoadMetadata() {
  using namespace Mantid::API;
  using namespace Mantid::Geometry;

  AnalysisDataService::Instance().remove("MD_EVENT_WS_ID");

  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("LoadSQW");
  alg->initialize();
  alg->setPropertyValue("Filename", this->m_filename);
  alg->setProperty("MetadataOnly", true); // Don't load the events.
  alg->setPropertyValue("OutputWorkspace", "MD_EVENT_WS_ID");
  alg->execute();

  Workspace_sptr result =
      AnalysisDataService::Instance().retrieve("MD_EVENT_WS_ID");
  Mantid::API::IMDEventWorkspace_sptr eventWs =
      boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(result);
  m_wsTypeName = eventWs->id();
  // Call base-class extraction method.
  extractMetadata(*eventWs);
}

/// Destructor
SQWLoadingPresenter::~SQWLoadingPresenter() {}

/*
Getter for the workspace type name.
@return Workspace Type Name
*/
std::string SQWLoadingPresenter::getWorkspaceTypeName() { return m_wsTypeName; }
} // namespace VATES
} // namespace Mantid

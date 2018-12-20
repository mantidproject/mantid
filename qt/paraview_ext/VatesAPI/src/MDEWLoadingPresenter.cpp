#include "MantidVatesAPI/MDEWLoadingPresenter.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidVatesAPI/MDLoadingView.h"

#include "MantidGeometry/MDGeometry/NullImplicitFunction.h"
#include "MantidVatesAPI/Common.h"
#include "MantidVatesAPI/MetaDataExtractorUtils.h"
#include "MantidVatesAPI/MetadataJsonManager.h"
#include "MantidVatesAPI/MetadataToFieldData.h"
#include "MantidVatesAPI/VatesConfigurations.h"
#include "MantidVatesAPI/VatesKnowledgeSerializer.h"
#include "MantidVatesAPI/VatesXMLDefinitions.h"

#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>
#include <vtkDataSet.h>
#include <vtkFieldData.h>

namespace {
Mantid::Kernel::Logger g_log("MDEWLoadingPresenter");
}

namespace Mantid {
namespace VATES {
/// Constructor
MDEWLoadingPresenter::MDEWLoadingPresenter(std::unique_ptr<MDLoadingView> view)
    : m_view(std::move(view)), m_isSetup(false), m_time(-1),
      m_recursionDepth(0), m_loadInMemory(false), m_firstLoad(true),
      m_metadataJsonManager(new MetadataJsonManager()),
      m_metaDataExtractor(new MetaDataExtractorUtils()),
      m_vatesConfigurations(new VatesConfigurations()) {
  Mantid::API::FrameworkManager::Instance();
}

/// Destructor
MDEWLoadingPresenter::~MDEWLoadingPresenter() {}

/*
Extract the geometry and function information
@param eventWs : event workspace to get the information from.
*/
void MDEWLoadingPresenter::extractMetadata(
    const Mantid::API::IMDEventWorkspace &eventWs) {
  using namespace Mantid::Geometry;
  MDGeometryBuilderXML<NoDimensionPolicy> refresh;
  xmlBuilder = refresh; // Reassign.
  std::vector<MDDimensionExtents<coord_t>> ext = eventWs.getMinimumExtents(5);
  std::vector<IMDDimension_sptr> dimensions;
  size_t nDimensions = eventWs.getNumDims();
  for (size_t d = 0; d < nDimensions; d++) {
    IMDDimension_const_sptr inDim = eventWs.getDimension(d);
    coord_t min = ext[d].getMin();
    coord_t max = ext[d].getMax();
    if (min > max) {
      min = 0.0;
      max = 1.0;
    }
    // std::cout << "dim " << d << min << " to " <<  max << '\n';
    axisLabels.push_back(makeAxisTitle(*inDim));
    dimensions.push_back(boost::make_shared<MDHistoDimension>(
        inDim->getName(), inDim->getName(), inDim->getMDFrame(), min, max,
        inDim->getNBins()));
  }

  // Configuring the geometry xml builder allows the object panel associated
  // with this reader to later
  // determine how to display all geometry related properties.
  if (nDimensions > 0) {
    xmlBuilder.addXDimension(dimensions[0]);
  }
  if (nDimensions > 1) {
    xmlBuilder.addYDimension(dimensions[1]);
  }
  if (nDimensions > 2) {
    xmlBuilder.addZDimension(dimensions[2]);
  }
  if (nDimensions > 3) {
    tDimension = dimensions[3];
    xmlBuilder.addTDimension(tDimension);
  }
  m_isSetup = true;
}

/**
Method determines whether loading/re-loading is necessary.
*/
bool MDEWLoadingPresenter::shouldLoad() {
  double viewTime = m_view->getTime();
  size_t viewDepth = m_view->getRecursionDepth();
  bool viewLoadInMemory = m_view->getLoadInMemory();

  bool bExecute = false;
  if (m_time != viewTime) {
    bExecute = false; // Time has changed. This DOES NOT require reloading.
  }
  if (m_recursionDepth != viewDepth) {
    bExecute = false; // Recursion depth has changed. This is a vtkDataSet
                      // factory concern.
  }
  if (m_loadInMemory != viewLoadInMemory) {
    bExecute = true; // Must reload with memory/file option.
  }
  if (m_firstLoad) {
    bExecute = true; // First time round. should execute underlying algorithm.
  }

  // Save state.
  m_time = viewTime;
  m_recursionDepth = viewDepth;
  m_loadInMemory = viewLoadInMemory;
  m_firstLoad = false;
  // Return decision.
  return bExecute;
}

/**
Determines wheter the file can be loaded based on it's extension.
@param filename containing the extension
@param expectedExtension expected extension for the file to have
@return TRUE, only if the extension is approved.
*/
bool MDEWLoadingPresenter::canLoadFileBasedOnExtension(
    const std::string &filename, const std::string &expectedExtension) const {
  // Quick check based on extension.
  const size_t startExtension = filename.find_last_of('.');
  const size_t endExtension = filename.length();
  std::string extension =
      filename.substr(startExtension, endExtension - startExtension);
  boost::algorithm::to_lower(extension);
  boost::algorithm::trim(extension);
  return extension == expectedExtension;
}

/*
Append the geometry and function information onto the outgoing vtkDataSet.
@param visualDataSet : outgoing dataset on which to append metadata.
@param wsName : name of the workspace.
*/
void MDEWLoadingPresenter::appendMetadata(vtkDataSet *visualDataSet,
                                          const std::string &wsName) {
  using namespace Mantid::API;

  vtkNew<vtkFieldData> outputFD;

  // Serialize metadata
  VatesKnowledgeSerializer serializer;
  serializer.setWorkspaceName(wsName);
  serializer.setGeometryXML(xmlBuilder.create());
  serializer.setImplicitFunction(
      boost::make_shared<Mantid::Geometry::NullImplicitFunction>());
  std::string xmlString = serializer.createXMLString();

  // Serialize Json metadata
  std::string jsonString = m_metadataJsonManager->getSerializedJson();

  // Add metadata to dataset.
  MetadataToFieldData convert;
  convert(outputFD.GetPointer(), xmlString, XMLDefinitions::metaDataId());
  convert(outputFD.GetPointer(), jsonString,
          m_vatesConfigurations->getMetadataIdJson());
  visualDataSet->SetFieldData(outputFD.GetPointer());
}

/**
 * Set the axis labels from the current dimensions
 * @param visualDataSet: The VTK dataset to update
 */
void MDEWLoadingPresenter::setAxisLabels(vtkDataSet *visualDataSet) {
  if (!vtkPVChangeOfBasisHelper::AddBasisNames(
          visualDataSet, axisLabels[0].c_str(), axisLabels[1].c_str(),
          axisLabels[2].c_str())) {
    g_log.warning("The basis names could not be added to the field data of "
                  "the data set.\n");
  }
}

/**
Gets the geometry in a string format.
@return geometry string ref.
@throw runtime_error if execute has not been run first.
*/
const std::string &MDEWLoadingPresenter::getGeometryXML() const {
  if (!m_isSetup) {
    throw std::runtime_error("Have not yet run extractMetaData!");
  }
  return xmlBuilder.create();
}

/**
@return boolean indicating whether the T dimension is available.
@throw runtime_error if execute has not been run first.
*/
bool MDEWLoadingPresenter::hasTDimensionAvailable() const {
  if (!m_isSetup) {
    throw std::runtime_error("Have not yet run ::extractMetaData!");
  }
  return xmlBuilder.hasTDimension() && !xmlBuilder.hasIntegratedTDimension();
}

/*
@return timestep values.
@throw runtime_error if execute has not been run first.
*/
std::vector<double> MDEWLoadingPresenter::getTimeStepValues() const {
  if (!m_isSetup) {
    throw std::runtime_error("Have not yet run ::extractMetaData!");
  }
  std::vector<double> result;
  for (size_t i = 0; i < tDimension->getNBins(); i++) {
    coord_t bin_centre =
        coord_t((tDimension->getX(i) + tDimension->getX(i + 1)) * 0.5);
    result.push_back(bin_centre);
  }
  return result;
}

/**
 * Create a label for the "time" coordinate
 * @return the "time" coordinate label
 * @throw runtime_error if execute has not been run first.
 */
std::string MDEWLoadingPresenter::getTimeStepLabel() const {
  if (!m_isSetup) {
    throw std::runtime_error("Have not yet run ::extractMetaData!");
  }
  return tDimension->getName() + " (" + tDimension->getUnits().ascii() + ")";
}

/**
 * Getter for the instrument.
 * @returns The name of the instrument which is associated with the workspace.
 */
const std::string &MDEWLoadingPresenter::getInstrument() {
  return m_metadataJsonManager->getInstrument();
}
} // namespace VATES
} // namespace Mantid

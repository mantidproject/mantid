#include "MantidVatesAPI/MDHWLoadingPresenter.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidVatesAPI/MDLoadingView.h"

#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/NullImplicitFunction.h"
#include "MantidVatesAPI/Common.h"
#include "MantidVatesAPI/MetaDataExtractorUtils.h"
#include "MantidVatesAPI/MetadataJsonManager.h"
#include "MantidVatesAPI/MetadataToFieldData.h"
#include "MantidVatesAPI/VatesConfigurations.h"
#include "MantidVatesAPI/VatesKnowledgeSerializer.h"
#include "MantidVatesAPI/VatesXMLDefinitions.h"
#include "MantidVatesAPI/vtkDataSetToNonOrthogonalDataSet.h"
#include "MantidVatesAPI/vtkDataSetToWsName.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/scoped_ptr.hpp>

#include <vtkDataSet.h>
#include <vtkFieldData.h>
#include <vtkPVChangeOfBasisHelper.h>

namespace {
Mantid::Kernel::Logger g_log("MDHWLoadingPresenter");
}

namespace Mantid {
namespace VATES {

/**
 * @brief MDHWInMemoryLoadingPresenter::transposeWs
 *
 * vtkDataSets are usually provided in 3D, trying to create these where one of
 *those dimensions
 * might be integrated out leads to empty datasets. To avoid this we reorder the
 *dimensions in our workspace
 * prior to visualisation by transposing if if needed.
 *
 * @param inHistoWs : An input workspace that may integrated dimensions
 *anywhere.
 * @param outCachedHistoWs : Cached histo workspace. To write to if needed.
 * @return A workspace that can be directly rendered from. Integrated dimensions
 *are always last.
 */
void MDHWLoadingPresenter::transposeWs(
    Mantid::API::IMDHistoWorkspace_sptr &inHistoWs,
    Mantid::API::IMDHistoWorkspace_sptr &outCachedHistoWs) {
  using namespace Mantid::API;

  if (!outCachedHistoWs) {
    /*
     Construct dimension indexes list for transpose. We do this by forcing
     integrated
     dimensions to be the last in the list. All other orderings are kept.
     */
    std::vector<int> integratedDims;
    std::vector<int> nonIntegratedDims;
    for (int i = 0; i < int(inHistoWs->getNumDims()); ++i) {
      auto dim = inHistoWs->getDimension(i);
      if (dim->getIsIntegrated()) {
        integratedDims.push_back(i);
      } else {
        nonIntegratedDims.push_back(i);
      }
    }

    std::vector<int> orderedDims = nonIntegratedDims;
    orderedDims.insert(orderedDims.end(), integratedDims.begin(),
                       integratedDims.end());

    /*
     If there has been any reordering above, then the dimension indexes will
     no longer be sorted. We use that to determine if we can avoid transposing
     the workspace.
     */
    if (!std::is_sorted(orderedDims.begin(), orderedDims.end())) {
      IAlgorithm_sptr alg = AlgorithmManager::Instance().create("TransposeMD");
      alg->setChild(true);
      alg->initialize();
      alg->setProperty("InputWorkspace", inHistoWs);
      alg->setPropertyValue("OutputWorkspace", "dummy");
      alg->setProperty("Axes", orderedDims);
      alg->execute();
      IMDHistoWorkspace_sptr visualHistoWs =
          alg->getProperty("OutputWorkspace");
      outCachedHistoWs = visualHistoWs;
    } else {
      // No need to transpose anything.
      outCachedHistoWs = inHistoWs;
    }
  }
}

/// Constructor
MDHWLoadingPresenter::MDHWLoadingPresenter(std::unique_ptr<MDLoadingView> view)
    : m_view(std::move(view)), m_isSetup(false), m_time(-1),
      m_loadInMemory(false), m_firstLoad(true),
      m_metadataJsonManager(new MetadataJsonManager()),
      m_metaDataExtractor(new MetaDataExtractorUtils()),
      m_vatesConfigurations(new VatesConfigurations()) {
  Mantid::API::FrameworkManager::Instance();
}

/// Destructor
MDHWLoadingPresenter::~MDHWLoadingPresenter() {}

/*
Extract the geometry and function information
@param histoWs : histogram workspace to get the information from.
*/
void MDHWLoadingPresenter::extractMetadata(
    const Mantid::API::IMDHistoWorkspace &histoWs) {
  using namespace Mantid::Geometry;
  MDGeometryBuilderXML<NoDimensionPolicy> refresh;
  xmlBuilder = refresh; // Reassign.
  std::vector<IMDDimension_sptr> dimensions;
  size_t nDimensions = histoWs.getNumDims();
  for (size_t d = 0; d < nDimensions; d++) {
    IMDDimension_const_sptr inDim = histoWs.getDimension(d);
    coord_t min = inDim->getMinimum();
    coord_t max = inDim->getMaximum();
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
bool MDHWLoadingPresenter::shouldLoad() {
  double viewTime = m_view->getTime();
  bool viewLoadInMemory = m_view->getLoadInMemory();

  bool bExecute = false;
  if (m_time != viewTime) {
    bExecute = false; // Time has changed. This DOES NOT require reloading.
  }
  if (m_loadInMemory != viewLoadInMemory) {
    bExecute = true; // Must reload with memory/file option.
  }
  if (m_firstLoad) {
    bExecute = true; // First time round. should execute underlying algorithm.
  }

  // Save state.
  m_time = viewTime;
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
bool MDHWLoadingPresenter::canLoadFileBasedOnExtension(
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
void MDHWLoadingPresenter::appendMetadata(vtkDataSet *visualDataSet,
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
void MDHWLoadingPresenter::setAxisLabels(vtkDataSet *visualDataSet) {
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
const std::string &MDHWLoadingPresenter::getGeometryXML() const {
  if (!m_isSetup) {
    throw std::runtime_error("Have not yet run extractMetaData!");
  }
  return xmlBuilder.create();
}

/**
@return boolean indicating whether the T dimension is available.
@throw runtime_error if execute has not been run first.
*/
bool MDHWLoadingPresenter::hasTDimensionAvailable() const {
  if (!m_isSetup) {
    throw std::runtime_error("Have not yet run ::extractMetaData!");
  }
  return xmlBuilder.hasTDimension() && !xmlBuilder.hasIntegratedTDimension();
}

/*
@return timestep values.
@throw runtime_error if execute has not been run first.
*/
std::vector<double> MDHWLoadingPresenter::getTimeStepValues() const {
  if (!m_isSetup) {
    throw std::runtime_error("Have not yet run ::extractMetaData!");
  }
  std::vector<double> result;
  for (size_t i = 0; i < tDimension->getNBins(); i++) {
    result.push_back(tDimension->getX(i));
  }
  return result;
}

/**
 * Create a label for the "time" coordinate
 * @return the "time" coordinate label
 * @throw runtime_error if execute has not been run first.
 */
std::string MDHWLoadingPresenter::getTimeStepLabel() const {
  if (!m_isSetup) {
    throw std::runtime_error("Have not yet run ::extractMetaData!");
  }
  return tDimension->getName() + " (" + tDimension->getUnits().ascii() + ")";
}

/**
 * Getter for the instrument.
 * @returns The name of the instrument which is associated with the workspace.
 */
const std::string &MDHWLoadingPresenter::getInstrument() {
  return m_metadataJsonManager->getInstrument();
}
} // namespace VATES
} // namespace Mantid

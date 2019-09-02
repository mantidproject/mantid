#include "vtkSplatterPlot.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGridAlgorithm.h"
#include "vtkUnstructuredGrid.h"
#include "vtkFieldData.h"
#include "vtkSmartPointer.h"

#include "MantidGeometry/MDGeometry/MDGeometryXMLDefinitions.h"
#include "MantidVatesAPI/ADSWorkspaceProvider.h"
#include "MantidVatesAPI/FieldDataToMetadata.h"
#include "MantidVatesAPI/FilteringUpdateProgressAction.h"
#include "MantidVatesAPI/VatesXMLDefinitions.h"
#include "MantidVatesAPI/vtkDataSetToNonOrthogonalDataSet.h"
#include "MantidVatesAPI/vtkDataSetToWsName.h"
#include "MantidVatesAPI/vtkSplatterPlotFactory.h"

using namespace Mantid::API;
using namespace Mantid::VATES;

vtkStandardNewMacro(vtkSplatterPlot)

    /// Constructor
    vtkSplatterPlot::vtkSplatterPlot()
    : m_numberPoints(0), m_topPercentile(0.0), m_wsName(""), m_time(0) {
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

/// Destructor
vtkSplatterPlot::~vtkSplatterPlot() {}

/**
 * Sets number of points.
 * @param nPoints : number of points.
 */
void vtkSplatterPlot::SetNumberOfPoints(int nPoints)
{
  if(nPoints >= 0)
  {
    size_t temp = static_cast<size_t>(nPoints);
    if(m_numberPoints != temp)
    {
      m_numberPoints = temp;
      if (nullptr != m_presenter) {
        m_presenter->SetNumberOfPoints(m_numberPoints);
      }
      this->Modified();
    }
  }
}

/**
 * Set the threshold for the top percentile of most dense boxes to view
 * @param topPercentile : the limit on the percentile of boxes to show
 */
void vtkSplatterPlot::SetTopPercentile(double topPercentile)
{
  if (topPercentile > 0)
  {
    if (m_topPercentile != topPercentile)
    {
      m_topPercentile = topPercentile;
      if (nullptr != m_presenter) {
        m_presenter->SetPercentToUse(m_topPercentile);
      }
      this->Modified();
    }
  }
}

/**
 * Getter for the time
 * @return the time.
 */
double vtkSplatterPlot::getTime() const
{
  return m_time;
}

int vtkSplatterPlot::RequestData(vtkInformation * info,
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector)
{
  if (nullptr == m_presenter) {
    /// If the presenter is not already setup attempt to set it up now
    /// this might fail, which is handled by the next selection statement
    this->RequestInformation(info, inputVector, outputVector);
  }

  if (nullptr != m_presenter) {
    // Get the info objects
    vtkInformation *outInfo = outputVector->GetInformationObject(0);
    vtkDataSet *output =
        vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
    {
      // usually only one actual step requested
      m_time = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    }
    m_presenter->setTime(m_time);

    vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
    vtkDataSet *input = vtkDataSet::SafeDownCast(
          inInfo->Get(vtkDataObject::DATA_OBJECT()));

    FilterUpdateProgressAction<vtkSplatterPlot> drawUpdateProgress(this,
                                                                   "Drawing...");
    auto product = m_presenter->create(drawUpdateProgress);

    // Extract the relevant metadata from the underlying source
    m_presenter->setMetadata(input->GetFieldData(), product);

    output->ShallowCopy(product);

    try
    {
      auto workspaceProvider = std::make_unique<ADSWorkspaceProvider<Mantid::API::IMDWorkspace>>();
      vtkDataSetToNonOrthogonalDataSet converter(output, m_wsName, std::move(workspaceProvider));
      converter.execute();
    }
    catch (std::invalid_argument &e)
    {
      std::string error = e.what();
      vtkDebugMacro(<< "Workspace does not have correct information to "
                    << "plot non-orthogonal axes. " << error);
    }
  }
  return 1;
}

int vtkSplatterPlot::RequestInformation(vtkInformation *,
                                        vtkInformationVector **inputVector,
                                        vtkInformationVector *)
{
  if (nullptr == m_presenter) {

    try {
      std::string scalarName = "signal";
      m_presenter = std::make_unique<vtkSplatterPlotFactory>(
          scalarName, m_numberPoints, m_topPercentile);

      // Get the info objects
      vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
      vtkDataSet *input = vtkDataSet::SafeDownCast(
            inInfo->Get(vtkDataObject::DATA_OBJECT()));
      m_wsName = Mantid::VATES::vtkDataSetToWsName::exec(input);
      // Get the workspace from the ADS
      ADSWorkspaceProvider<IMDWorkspace> workspaceProvider;
      Workspace_sptr result = workspaceProvider.fetchWorkspace(m_wsName);
      m_presenter->initialize(result);
    } catch (const std::runtime_error &) {
      // Catch incase something goes wrong. It might be that the splatter
      // plot source is not yet setup correctly and we'll need to run this
      // call again later.
      m_presenter = nullptr;
    }

  }
  return 1;
}

void vtkSplatterPlot::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

/**
 * Output the progress information and progress text.
 * @param : progress
 * @param : message
 */
void vtkSplatterPlot::updateAlgorithmProgress(double progress, const std::string& message)
{
  this->SetProgressText(message.c_str());
  this->UpdateProgress(progress);
}

/**
 * Gets the (first) instrument which is associated with the workspace.
 * @return The name of the instrument.
 */
const char* vtkSplatterPlot::GetInstrument()
{
  if (nullptr == m_presenter) {
    return "";
  }
  try {
    return m_presenter->getInstrument().c_str();
  }
  catch (std::runtime_error &)
  {
    return "";
  }
}


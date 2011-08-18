#include "MantidVatesAPI/MDEWRebinningPresenter.h"
#include "MantidVatesAPI/NullRebinningPresenter.h"

#include "vtkMDEWRebinningCutter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkAlgorithm.h"
#include "vtkPVClipDataSet.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkImplicitFunction.h"
#include "vtkPointData.h"
#include "vtkBox.h"

#include "MantidKernel/Exception.h"
#include "MantidMDAlgorithms/PlaneImplicitFunction.h"
#include "MantidMDAlgorithms/BoxImplicitFunction.h"
#include "MantidMDAlgorithms/NullImplicitFunction.h"
#include "MantidGeometry/MDGeometry/IMDDimensionFactory.h"
#include "MantidVatesAPI/EscalatingRebinningActionManager.h"
#include "MantidVatesAPI/RebinningCutterXMLDefinitions.h"
#include "MantidVatesAPI/vtkThresholdingUnstructuredGridFactory.h"
#include "MantidVatesAPI/vtkThresholdingHexahedronFactory.h"
#include "MantidVatesAPI/vtkThresholdingQuadFactory.h"
#include "MantidVatesAPI/vtkThresholdingLineFactory.h"
#include "MantidVatesAPI/IMDWorkspaceProxy.h"
#include "MantidVatesAPI/vtkProxyFactory.h"
#include "MantidVatesAPI/TimeToTimeStep.h"
#include "MantidVatesAPI/FilteringUpdateProgressAction.h"
#include "MantidVatesAPI/Common.h"
#include "MantidVatesAPI/vtkDataSetToGeometry.h" 
#include "MantidVatesAPI/GaussianThresholdRange.h"
#include "MantidVatesAPI/UserDefinedThresholdRange.h"
#include "MantidVatesAPI/NoThresholdRange.h"
#include "MantidVatesAPI/IgnoreZerosThresholdRange.h"
#include "MantidVatesAPI/MedianAndBelowThresholdRange.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLParser.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"

#include <boost/functional/hash.hpp>
#include <sstream>

/** Plugin for ParaView. Performs simultaneous rebinning and slicing of Mantid data.

@author Owen Arnold, Tessella plc
@date 14/03/2011

Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

/** Getter for the implicit function
@return The implicit function
*/
vtkImplicitFunction* vtkMDEWRebinningCutter::getImplicitFunction() const
{
  return m_clipFunction;
}

/** Getter for the max threshold
@return max threshold
*/
double vtkMDEWRebinningCutter::getMaxThreshold() const
{
  return m_thresholdMax;
}

/** Getter for the min threshold
@return min threshold
*/
double vtkMDEWRebinningCutter::getMinThreshold() const
{
  return m_thresholdMin;
}

/** Getter flag indicating wheter clipping is applied.
*/
bool vtkMDEWRebinningCutter::getApplyClip() const
{
  return m_clip == ApplyClipping;
}

/** Getter for the timestep
@return timestep value
*/
double vtkMDEWRebinningCutter::getTimeStep() const
{
  return m_timestep;
}

/** Getter for applied geometry xml.
@return ptr to applied geometry string.
*/
const char* vtkMDEWRebinningCutter::getAppliedGeometryXML() const
{
  return m_appliedGeometryXML.c_str();
}

/** Setter for the algorithm progress..
@parameter progress
*/
void vtkMDEWRebinningCutter::updateAlgorithmProgress(double progress)
{
  progressMutex.lock();
  this->SetProgressText("Executing Mantid MDEvent Rebinning Algorithm...");
  this->UpdateProgress(progress);
  progressMutex.unlock();
}

/**
Getter for the implicit function width.
@return width
*/
double vtkMDEWRebinningCutter::getWidth() const
{
  return this->m_width;
}


vtkCxxRevisionMacro(vtkMDEWRebinningCutter, "$Revision: 1.0 $")
  ;
vtkStandardNewMacro(vtkMDEWRebinningCutter)
  ;

using namespace Mantid::VATES;

///Constructor.
vtkMDEWRebinningCutter::vtkMDEWRebinningCutter() :
m_presenter(new NullRebinningPresenter()),
  m_clipFunction(NULL),
  m_clip(ApplyClipping),
  m_originalExtents(IgnoreOriginal),
  m_setup(Pending),
  m_timestep(0),
  m_thresholdMax(1e9),
  m_thresholdMin(0),
  m_thresholdMethodIndex(0)
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

///Destructor.
vtkMDEWRebinningCutter::~vtkMDEWRebinningCutter()
{
  delete m_presenter;
}

void vtkMDEWRebinningCutter::configureThresholdRangeMethod()
{
  switch(m_thresholdMethodIndex)
  {
  case 0:
    m_ThresholdRange = ThresholdRange_scptr(new IgnoreZerosThresholdRange());
    break;
  case 1:
    m_ThresholdRange = ThresholdRange_scptr(new NoThresholdRange());
    break;
  case 2:
    m_ThresholdRange = ThresholdRange_scptr(new MedianAndBelowThresholdRange());
    break;
  case 3:
    m_ThresholdRange = ThresholdRange_scptr(new UserDefinedThresholdRange(m_thresholdMin, m_thresholdMax));
    break;
  }
}

int vtkMDEWRebinningCutter::RequestData(vtkInformation* vtkNotUsed(request), vtkInformationVector**,
  vtkInformationVector *outputVector)
{
  using namespace Mantid::VATES;

  //Setup is not complete until metadata has been correctly provided.
  if(SetupDone == m_setup)
  {
    configureThresholdRangeMethod();

    //Updating again at this point is the only way to pick-up changes to clipping.
    m_presenter->updateModel();

    FilterUpdateProgressAction<vtkMDEWRebinningCutter> updatehandler(this);

    vtkInformation *outInfo = outputVector->GetInformationObject(0);
    vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(outInfo->Get(
      vtkDataObject::DATA_OBJECT()));

    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
    {
      // usually only one actual step requested
      m_timestep = static_cast<int>( outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS())[0] );
    }

    //Create chain-of-responsibility for translating imdworkspaces.
    std::string scalarName = XMLDefinitions::signalName();
    vtkThresholdingLineFactory* vtkGridFactory = new vtkThresholdingLineFactory(m_ThresholdRange, scalarName);
    vtkThresholdingQuadFactory* p_2dSuccessorFactory = new vtkThresholdingQuadFactory(m_ThresholdRange,scalarName);
    vtkThresholdingHexahedronFactory* p_3dSuccessorFactory = new vtkThresholdingHexahedronFactory(m_ThresholdRange,scalarName);
    vtkThresholdingUnstructuredGridFactory<TimeToTimeStep>* p_4dSuccessorFactory = new vtkThresholdingUnstructuredGridFactory<TimeToTimeStep>(m_ThresholdRange,scalarName, m_timestep);
    vtkGridFactory->SetSuccessor(p_2dSuccessorFactory);
    p_2dSuccessorFactory->SetSuccessor(p_3dSuccessorFactory);
    p_3dSuccessorFactory->SetSuccessor(p_4dSuccessorFactory);

    vtkDataSet* outData = m_presenter->execute(vtkGridFactory, updatehandler);
    m_thresholdMax = m_ThresholdRange->getMaximum();
    m_thresholdMin = m_ThresholdRange->getMinimum();
    delete vtkGridFactory;

    output->ShallowCopy(outData);

  }
  return 1;
}

int vtkMDEWRebinningCutter::RequestInformation(vtkInformation* vtkNotUsed(request), vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  using namespace Mantid::VATES;

  enum Status{Bad=0, Good=1};
  Status status=Good;
  if (Pending == m_setup)
  {
    vtkInformation * inputInf = inputVector[0]->GetInformationObject(0);
    vtkDataSet * inputDataset = vtkDataSet::SafeDownCast(inputInf->Get(vtkDataObject::DATA_OBJECT()));

    using namespace Mantid::VATES;

    RebinningActionManager* requester = new EscalatingRebinningActionManager();
    requester->ask(RecalculateAll);
    Mantid::VATES::MDEWRebinningPresenter<vtkMDEWRebinningCutter>* temp;
    try
    {
      temp = new MDEWRebinningPresenter<vtkMDEWRebinningCutter>(inputDataset, requester, this);
      delete this->m_presenter;
      m_presenter = temp;
      m_appliedGeometryXML = m_presenter->getAppliedGeometryXML();
      m_setup = SetupDone;
    }
    catch(std::logic_error&)
    {
      vtkErrorMacro("Rebinning operations require Rebinning Metadata. Have you provided a rebinning source?");
      status = Bad;
    }
    setTimeRange(outputVector);
  }
  return status;
}

int vtkMDEWRebinningCutter::RequestUpdateExtent(vtkInformation* vtkNotUsed(info), vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* vtkNotUsed(outputVector))
{
  return 1;
}
;

int vtkMDEWRebinningCutter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

void vtkMDEWRebinningCutter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkMDEWRebinningCutter::SetApplyClip(int applyClip)
{
  Clipping temp = applyClip == 1 ? ApplyClipping : IgnoreClipping;
  if(temp != m_clip)
  {
    m_clip = temp;
    this->Modified();
  }
}

void vtkMDEWRebinningCutter::SetWidth(double width)
{
  if(m_width != width)
  {
    m_width = width;
    this->Modified();
  }
}

void vtkMDEWRebinningCutter::SetClipFunction(vtkImplicitFunction * func)
{
  vtkPlane* plane = dynamic_cast<vtkPlane*>(func);
  if (plane != m_clipFunction)
  {
    this->m_clipFunction = plane;
    this->Modified();
  }
}

void vtkMDEWRebinningCutter::SetMaxThreshold(double maxThreshold)
{
  if (maxThreshold != m_thresholdMax)
  {
    this->m_thresholdMax = maxThreshold;
    this->Modified();
  }
}

void vtkMDEWRebinningCutter::SetMinThreshold(double minThreshold)
{
  if (minThreshold != m_thresholdMin)
  {
    this->m_thresholdMin = minThreshold;
    this->Modified();
  }
}


void vtkMDEWRebinningCutter::SetAppliedGeometryXML(std::string appliedGeometryXML)
{
  if(SetupDone == m_setup)
  {
    m_appliedGeometryXML = appliedGeometryXML;
    this->Modified(); 
  }
}

void vtkMDEWRebinningCutter::SetThresholdRangeStrategyIndex(std::string selectedStrategyIndex)
{
  int index = atoi(selectedStrategyIndex.c_str());
  if(index != m_thresholdMethodIndex)
  {
    m_thresholdMethodIndex = index;
    this->Modified();
  }
}

const char* vtkMDEWRebinningCutter::GetInputGeometryXML()
{
  try
  {
    return this->m_presenter->getAppliedGeometryXML().c_str(); //TODO, check xml lives beyond function call.
  }
  catch(std::runtime_error&)
  {
    return "";
  }
}

double vtkMDEWRebinningCutter::GetInputMinThreshold()
{
  return m_thresholdMin;
}

double vtkMDEWRebinningCutter::GetInputMaxThreshold()
{
  return m_thresholdMax;
}

unsigned long vtkMDEWRebinningCutter::GetMTime()
{
  unsigned long mTime = this->Superclass::GetMTime();
  unsigned long time;

  if (this->m_clipFunction != NULL)
  {

    time = this->m_clipFunction->GetMTime();
    if(time > mTime)
    {
      mTime = time;
    }
  }

  return mTime;
}



void vtkMDEWRebinningCutter::setTimeRange(vtkInformationVector* outputVector)
{
  if(SetupDone == m_setup)
  {
    if(m_presenter->hasTDimensionAvailable())
    {
      vtkInformation *outInfo = outputVector->GetInformationObject(0);
      std::vector<double> timeStepValues = m_presenter->getTimeStepValues();
      outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &timeStepValues[0],
        static_cast<int> (timeStepValues.size()));
      double timeRange[2];
      timeRange[0] = timeStepValues.front();
      timeRange[1] = timeStepValues.back();

      outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
    }
  }
}


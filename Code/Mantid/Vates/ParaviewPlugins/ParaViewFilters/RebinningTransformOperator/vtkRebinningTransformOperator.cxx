#include "MantidVatesAPI/MDEWRebinningPresenter.h"
#include "MantidVatesAPI/NullRebinningPresenter.h"

#include "vtkRebinningTransformOperator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkAlgorithm.h"
#include "vtkPVClipDataSet.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPointData.h"

#include "MantidKernel/Exception.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidVatesAPI/ADSWorkspaceProvider.h"
#include "MantidGeometry/MDGeometry/NullImplicitFunction.h"
#include "MantidVatesAPI/EscalatingRebinningActionManager.h"
#include "MantidVatesAPI/RebinningCutterXMLDefinitions.h"
#include "MantidVatesAPI/vtkMDQuadFactory.h"
#include "MantidVatesAPI/vtkMDLineFactory.h"
#include "MantidVatesAPI/vtkMDHexFactory.h"
#include "MantidVatesAPI/vtkMDHistoHex4DFactory.h"
#include "MantidVatesAPI/vtkMDHistoHexFactory.h"
#include "MantidVatesAPI/vtkMDHistoQuadFactory.h"
#include "MantidVatesAPI/vtkMDHistoLineFactory.h"
#include "MantidVatesAPI/TimeToTimeStep.h"
#include "MantidVatesAPI/FilteringUpdateProgressAction.h"
#include "MantidVatesAPI/Common.h"
#include "MantidVatesAPI/vtkDataSetToGeometry.h" 
#include "MantidVatesAPI/UserDefinedThresholdRange.h"
#include "MantidVatesAPI/NoThresholdRange.h"
#include "MantidVatesAPI/IgnoreZerosThresholdRange.h"
#include "MantidVatesAPI/MedianAndBelowThresholdRange.h"
#include "MantidVatesAPI/ADSWorkspaceProvider.h"
#include "MantidVatesAPI/MDRebinningViewAdapter.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLParser.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"


#include <boost/functional/hash.hpp>
#include <sstream>


#include "MantidVatesAPI/Clipper.h"
#include <vtkPVClipDataSet.h>

class ClipperAdapter : public Mantid::VATES::Clipper
{
private:
  vtkPVClipDataSet* m_clipper;
public:

  ClipperAdapter(vtkPVClipDataSet* pClipper) : m_clipper(pClipper)
  {
  }

  void SetInput(vtkDataSet* input)
  {
    m_clipper->SetInputData(input);
  }

  void SetClipFunction(vtkImplicitFunction* func)
  {
    m_clipper->SetClipFunction(func);
  }

  void SetInsideOut(bool insideout)
  {
    m_clipper->SetInsideOut(insideout);
  }

  void SetRemoveWholeCells(bool) 
  {
  }

  void SetOutput(vtkUnstructuredGrid* out_ds)
  {
    m_clipper->SetOutput(out_ds);
  }

  void Update()
  {
    m_clipper->Update();
  }

  void Delete()
  {
    delete this;
  }

  ~ClipperAdapter()
  {
    m_clipper->Delete();
  }

  vtkDataSet* GetOutput()
  {
    return m_clipper->GetOutput();
  }

};


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

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/


/** Getter for the max threshold
@return max threshold
*/
double vtkRebinningTransformOperator::getMaxThreshold() const
{
  return m_thresholdMax;
}

/** Getter for the min threshold
@return min threshold
*/
double vtkRebinningTransformOperator::getMinThreshold() const
{
  return m_thresholdMin;
}

/** Getter flag indicating wheter clipping is applied.
*/
bool vtkRebinningTransformOperator::getApplyClip() const
{
  return m_clip == ApplyClipping;
}

/** Getter for the timestep
@return timestep value
*/
double vtkRebinningTransformOperator::getTimeStep() const
{
  return m_timestep;
}

/** Getter for applied geometry xml.
@return ptr to applied geometry string.
*/
const char* vtkRebinningTransformOperator::getAppliedGeometryXML() const
{
  return m_appliedGeometryXML.c_str();
}

/** Setter for the algorithm progress..
@param progress : The current progress value
@param message : Progress message
*/
void vtkRebinningTransformOperator::updateAlgorithmProgress(double progress, const std::string& message)
{
  progressMutex.lock();
  this->SetProgressText(message.c_str());
  this->UpdateProgress(progress);
  progressMutex.unlock();
}

bool vtkRebinningTransformOperator::getOutputHistogramWS() const
{
  return m_bOutputHistogramWS;
}

vtkStandardNewMacro(vtkRebinningTransformOperator);

using namespace Mantid::VATES;

///Constructor.
vtkRebinningTransformOperator::vtkRebinningTransformOperator() :
m_presenter(new NullRebinningPresenter()),
  m_clip(ApplyClipping),
  m_originalExtents(IgnoreOriginal),
  m_setup(Pending),
  m_timestep(0),
  m_thresholdMax(1e9),
  m_thresholdMin(0),
  m_thresholdMethodIndex(0),
  m_origin(0, 0, 0),
  m_lengthB1(1),
  m_lengthB2(1),
  m_lengthB3(1),
  m_ForceOrthogonal(true),
  m_bOutputHistogramWS(true)
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

///Destructor.
vtkRebinningTransformOperator::~vtkRebinningTransformOperator()
{
}

/*
Determine the threshold range strategy to use.
*/
void vtkRebinningTransformOperator::configureThresholdRangeMethod()
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

int vtkRebinningTransformOperator::RequestData(vtkInformation* vtkNotUsed(request), vtkInformationVector**,
  vtkInformationVector *outputVector)
{
  using namespace Mantid::VATES;

  //Setup is not complete until metadata has been correctly provided.
  if(SetupDone == m_setup)
  {
    configureThresholdRangeMethod();

    //Updating again at this point is the only way to pick-up changes to clipping.
    m_presenter->updateModel();

    FilterUpdateProgressAction<vtkRebinningTransformOperator> rebinningProgressUpdate(this, "Rebinning...");
    FilterUpdateProgressAction<vtkRebinningTransformOperator> drawingProgressUpdate(this, "Drawing...");

    vtkInformation *outInfo = outputVector->GetInformationObject(0);
    vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(outInfo->Get(
      vtkDataObject::DATA_OBJECT()));

    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
    {
      // usually only one actual step requested
      m_timestep = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    }

    std::string scalarName = XMLDefinitions::signalName();

    //Create Factory Object for chain. Chain-of-responsibility for translating imdworkspaces.
    vtkMDLineFactory* p_1dMDFactory =  new vtkMDLineFactory(m_ThresholdRange, scalarName);
    vtkMDQuadFactory* p_2dMDFactory = new vtkMDQuadFactory(m_ThresholdRange, scalarName);
    vtkMDHexFactory* p_3dMDFactory = new vtkMDHexFactory(m_ThresholdRange, scalarName);
    vtkMDHistoLineFactory* p_1dHistoFactory = new vtkMDHistoLineFactory(m_ThresholdRange, scalarName);
    vtkMDHistoQuadFactory* p_2dHistoFactory = new vtkMDHistoQuadFactory(m_ThresholdRange,scalarName);
    vtkMDHistoHexFactory* p_3dHistoFactory = new vtkMDHistoHexFactory(m_ThresholdRange,scalarName);
    vtkMDHistoHex4DFactory<TimeToTimeStep>* p_4dHistoFactory = new vtkMDHistoHex4DFactory<TimeToTimeStep>(m_ThresholdRange,scalarName, m_timestep);

    //Assemble Chain-of-Reponsibility
    p_1dMDFactory->SetSuccessor(p_2dMDFactory);
    p_2dMDFactory->SetSuccessor(p_3dMDFactory);
    p_3dMDFactory->SetSuccessor(p_1dHistoFactory);
    p_1dHistoFactory->SetSuccessor(p_2dHistoFactory);
    p_2dHistoFactory->SetSuccessor(p_3dHistoFactory);
    p_3dHistoFactory->SetSuccessor(p_4dHistoFactory);

    vtkDataSet* outData = m_presenter->execute(p_1dMDFactory, rebinningProgressUpdate, drawingProgressUpdate);
    m_thresholdMax = m_ThresholdRange->getMaximum();
    m_thresholdMin = m_ThresholdRange->getMinimum();
    delete p_1dMDFactory;

    output->ShallowCopy(outData);
    m_presenter->setAxisLabels(output);
  }
  return 1;
}

int vtkRebinningTransformOperator::RequestInformation(vtkInformation* vtkNotUsed(request), vtkInformationVector **inputVector,
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

    //Try to use another type of presenter with this view. One for MDEWs.
    ADSWorkspaceProvider<Mantid::API::IMDEventWorkspace> wsProvider;
    MDRebinningPresenter_sptr temp= MDRebinningPresenter_sptr(new MDEWRebinningPresenter(inputDataset, new EscalatingRebinningActionManager(RecalculateAll), new MDRebinningViewAdapter<vtkRebinningTransformOperator>(this), wsProvider));
    m_presenter = temp;
    
    m_appliedGeometryXML = m_presenter->getAppliedGeometryXML();
    m_setup = SetupDone;
  }
  setTimeRange(outputVector);
  return status;
}

int vtkRebinningTransformOperator::RequestUpdateExtent(vtkInformation* vtkNotUsed(info), vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* vtkNotUsed(outputVector))
{
  return 1;
}
;

int vtkRebinningTransformOperator::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

void vtkRebinningTransformOperator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkRebinningTransformOperator::SetMaxThreshold(double maxThreshold)
{
  if (maxThreshold != m_thresholdMax)
  {
    this->m_thresholdMax = maxThreshold;
    this->Modified();
  }
}

void vtkRebinningTransformOperator::SetMinThreshold(double minThreshold)
{
  if (minThreshold != m_thresholdMin)
  {
    this->m_thresholdMin = minThreshold;
    this->Modified();
  }
}

void vtkRebinningTransformOperator::SetOutputHistogramWS(bool bOutputHistogramWS)
{
  if(bOutputHistogramWS != m_bOutputHistogramWS)
  {
    m_bOutputHistogramWS = bOutputHistogramWS;
    this->Modified();
  }
}


void vtkRebinningTransformOperator::SetAppliedGeometryXML(std::string appliedGeometryXML)
{
  if(SetupDone == m_setup)
  {
    m_appliedGeometryXML = appliedGeometryXML;
    this->Modified(); 
  }
}

void vtkRebinningTransformOperator::SetThresholdRangeStrategyIndex(std::string selectedStrategyIndex)
{
  int index = atoi(selectedStrategyIndex.c_str());
  if(index != m_thresholdMethodIndex)
  {
    m_thresholdMethodIndex = index;
    this->Modified();
  }
}

const char* vtkRebinningTransformOperator::GetInputGeometryXML()
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

double vtkRebinningTransformOperator::GetInputMinThreshold()
{
  return m_thresholdMin;
}

double vtkRebinningTransformOperator::GetInputMaxThreshold()
{
  return m_thresholdMax;
}

unsigned long vtkRebinningTransformOperator::GetMTime()
{
  return vtkUnstructuredGridAlgorithm::GetMTime();
}



void vtkRebinningTransformOperator::setTimeRange(vtkInformationVector* outputVector)
{
  if(SetupDone == m_setup)
  {
    if(m_presenter->hasTDimensionAvailable())
    {
      vtkInformation *outInfo = outputVector->GetInformationObject(0);
      outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_LABEL_ANNOTATION(),
                   m_presenter->getTimeStepLabel().c_str());
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

void vtkRebinningTransformOperator::SetB1(double a, double b, double c)
{
  Mantid::Kernel::V3D temp(a, b, c);
  if(m_b1 != temp)
  {
    m_b1 = temp;
    this->Modified();
  }
}

void vtkRebinningTransformOperator::SetB2(double a, double b, double c)
{
  Mantid::Kernel::V3D temp(a, b, c);
  if(m_b2 != temp)
  {
    m_b2 = temp;
    this->Modified();
  }
}

void vtkRebinningTransformOperator::SetLengthB1(double length)
{
  if(length != m_lengthB1)
  {
    m_lengthB1 = length;
    this->Modified();
  }
}

void vtkRebinningTransformOperator::SetLengthB2(double length)
{
  if(length != m_lengthB2)
  {
    m_lengthB2 = length;
    this->Modified();
  }
}

void vtkRebinningTransformOperator::SetLengthB3(double length)
{
  if(length != m_lengthB3)
  {
    m_lengthB3 = length;
    this->Modified();
  }
}

void vtkRebinningTransformOperator::SetOrigin(double originX, double originY, double originZ)
{
  Mantid::Kernel::V3D temp(originX, originY, originZ);
  if(temp != m_origin)
  {
    m_origin = temp;
    this->Modified();
  }
}

void vtkRebinningTransformOperator::SetForceOrthogonal(bool temp)
{
  if(temp != m_ForceOrthogonal)
  {
    m_ForceOrthogonal = temp;
    this->Modified();
  }
}


Mantid::Kernel::V3D vtkRebinningTransformOperator::getOrigin()
{
  return m_origin;
}

Mantid::Kernel::V3D vtkRebinningTransformOperator::getB1()
{
  return m_b1;
}

Mantid::Kernel::V3D vtkRebinningTransformOperator::getB2()
{
  return m_b2;
}

double vtkRebinningTransformOperator::getLengthB1() const
{
  return m_lengthB1;
}

double vtkRebinningTransformOperator::getLengthB2() const
{
  return m_lengthB2;
}

double vtkRebinningTransformOperator::getLengthB3() const
{
  return m_lengthB3;
}
bool vtkRebinningTransformOperator::getForceOrthogonal() const
{
  return m_ForceOrthogonal;
}

/**
 * Gets the minimum value of the data associated with the 
 * workspace.
 * @return The minimum value of the workspace data.
 */
double vtkRebinningTransformOperator::GetMinValue() const
{
  if (NULL == m_presenter)
  {
    return 0.0;
  }
  try
  {
    return m_presenter->getMinValue();
  }
  catch (std::runtime_error &)
  {
    return 0;
  }
}

/**
 * Gets the maximum value of the data associated with the 
 * workspace.
 * @return The maximum value of the workspace data.
 */
double vtkRebinningTransformOperator::GetMaxValue() const
{
  if (NULL == m_presenter)
  {
    return 0.0;
  }
  try
  {
    return m_presenter->getMaxValue();
  }
  catch (std::runtime_error &)
  {
    return 0;
  }
}

/**
 * Gets the (first) instrument which is associated with the workspace.
 * @return The name of the instrument.
 */
const char* vtkRebinningTransformOperator::GetInstrument() const
{
  if (NULL == m_presenter)
  {
    return "";
  }
  try
  {
    return m_presenter->getInstrument().c_str();
  }
  catch (std::runtime_error &)
  {
    return "";
  }
}

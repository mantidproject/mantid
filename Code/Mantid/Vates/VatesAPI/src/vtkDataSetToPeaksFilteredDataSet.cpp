#include "MantidVatesAPI/vtkDataSetToPeaksFilteredDataSet.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IPeak.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidDataObjects/NoShape.h"
#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidDataObjects/PeakShapeSpherical.h"
#include "MantidGeometry/Crystal/PeakShape.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/ReadLock.h"


#include <vtkExtractSelection.h>
#include <vtkIdTypeArray.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkSelection.h>
#include <vtkSelectionNode.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>


#include <boost/shared_ptr.hpp>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>


namespace Mantid
{
namespace VATES
{
  /**
    * Standard constructor for object.
    * @param input : The dataset to scale
    * @param output : The resulting scaled dataset
    */
  vtkDataSetToPeaksFilteredDataSet::vtkDataSetToPeaksFilteredDataSet(vtkUnstructuredGrid *input,
                                                                     vtkUnstructuredGrid *output) :
                                                              m_inputData(input),
                                                              m_outputData(output),
                                                              m_isInitialised(false),
                                                              m_radiusNoShape(1.0),
                                                              m_radiusType(0)
  {
    if (NULL == m_inputData)
    {
      throw std::runtime_error("Cannot construct vtkDataSetToScaledDataSet with NULL input vtkUnstructuredGrid");
    }
    if (NULL == m_outputData)
    {
      throw std::runtime_error("Cannot construct vtkDataSetToScaledDataSet with NULL output vtkUnstructuredGrid");
    }
  }


  vtkDataSetToPeaksFilteredDataSet::~vtkDataSetToPeaksFilteredDataSet()
  {
    
  }

  /**
    * Set the value for the underlying peaks workspace
    * @param peaksWorkspaceName : The name of the peaks workspace.
    * @param radiusNoShape : The peak radius for no shape.
    * @param radiusType : The type of the radius: Radius(0), Outer Radius(10, Inner Radius(1)
    */
  void vtkDataSetToPeaksFilteredDataSet::initialize(std::string peaksWorkspaceName, double radiusNoShape, int radiusType)
  {
    m_peaksWorkspaceName = peaksWorkspaceName;
    m_radiusNoShape = radiusNoShape;
    m_radiusType = radiusType;
    m_isInitialised = true;
  }

  /**
    * Process the input data. First, get all the peaks and their associated geometry. Then filter 
    * through the input to find the peaks which lie within a peak. Then apply then to the output data.
    * Then update the metadata. See http://www.vtk.org/Wiki/VTK/Examples/Cxx/PolyData/ExtractSelection
    */
  void vtkDataSetToPeaksFilteredDataSet::execute()
  {
    if (!m_isInitialised)
    {
      throw std::runtime_error("vtkDataSetToPeaksFilteredDataSet needs initialize run before executing");
    }

    // Get the peaks location and the radius information
    std::vector<std::pair<Mantid::Kernel::V3D, double>> peaksInfo = getPeaksInfo();

    // Compare each element of the vtk data set and check which ones to keep
    vtkPoints *points = m_inputData->GetPoints();

    vtkSmartPointer<vtkIdTypeArray> ids = vtkSmartPointer<vtkIdTypeArray>::New();
    ids->SetNumberOfComponents(1);

    //PRAGMA_OMP( parallel for schedule (dynamic) )
    for(int i = 0; i < points->GetNumberOfPoints(); i++)
    {
      double point[3];
      points->GetPoint(i, point);

      // Compare to Peaks
      const size_t numberOfPeaks = peaksInfo.size();
      size_t counter = 0;
      while (counter < numberOfPeaks)
      {
        // Calcuate the differnce between the vtkDataSet point and the peak. Needs to be smaller than the radius
        double squaredDifference = 0;
        for (int k = 0; k <3; k++)
        {
          squaredDifference += (point[k] - peaksInfo[counter].first[k])* (point[k] - peaksInfo[counter].first[k]);
        }

        if (squaredDifference <= (peaksInfo[counter].second*peaksInfo[counter].second))
        {
          ids->InsertNextValue(i);
          break;
        }
        counter++;
      }
    }

    // Create the selection node and tell it the type of selection
    vtkSmartPointer<vtkSelectionNode> selectionNode = vtkSmartPointer<vtkSelectionNode>::New();
    selectionNode->SetFieldType(vtkSelectionNode::CELL);
    selectionNode->SetContentType(vtkSelectionNode::INDICES);
    selectionNode->SetSelectionList(ids);

    vtkSmartPointer<vtkSelection> selection = vtkSmartPointer<vtkSelection>::New();
    selection->AddNode(selectionNode);

    // We are not setting up a pipeline here, cannot access vtkAlgorithmOutput
    vtkSmartPointer<vtkExtractSelection> extractSelection = vtkSmartPointer<vtkExtractSelection>::New();
    extractSelection->SetInputData(0,m_inputData);
    extractSelection->SetInputData(1, selection);
    extractSelection->Update();

    //Extract
    m_outputData->ShallowCopy(extractSelection->GetOutput());
  }

  /**
   * Get the peaks information 
   * @returns A list of peaks with position and radius information
   */
  std::vector<std::pair<Mantid::Kernel::V3D, double>> vtkDataSetToPeaksFilteredDataSet::getPeaksInfo()
  {
    // Get the peaks workps
    if (!Mantid::API::AnalysisDataService::Instance().doesExist(m_peaksWorkspaceName))
    {
      throw std::invalid_argument("The peaks workspace does not seem to exist.");
    }

    Mantid::API::IPeaksWorkspace_sptr peaksWorkspace = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::IPeaksWorkspace>(m_peaksWorkspaceName);
    Mantid::Kernel::ReadLock lock(*peaksWorkspace);
    const Mantid::Kernel::SpecialCoordinateSystem coordinateSystem = peaksWorkspace->getSpecialCoordinateSystem();

    // Iterate over all peaks and store their information
    std::vector<std::pair<Mantid::Kernel::V3D, double>> peaksInfo(peaksWorkspace->getNumberPeaks());

    for (int i = 0; i < peaksWorkspace->getNumberPeaks(); i++)
    {
      Mantid::API::IPeak* peak = peaksWorkspace->getPeakPtr(i);

      // Get radius, check directly for name and don't cast
      double radius;
      
      const Mantid::Geometry::PeakShape& shape = peak->getPeakShape();
      std::string shapeName = shape.shapeName();

      if (shapeName == "spherical")
      {
        const Mantid::DataObjects::PeakShapeSpherical& sphericalShape = dynamic_cast<const Mantid::DataObjects::PeakShapeSpherical&>(shape);
        if (m_radiusType == 0)
        {
          radius = sphericalShape.radius();
        }
        else if (m_radiusType == 1)
        {
           boost::optional<double> radOut = sphericalShape.backgroundOuterRadius();
           if (radOut) radius = *radOut;
        }
        else if (m_radiusType == 2)
        {
           boost::optional<double> radIn = sphericalShape.backgroundInnerRadius();
          if (radIn) radius = *radIn;
        }
        else 
        {
          throw std::invalid_argument("The shperical peak shape does not have a radius. \n");
        }
      }
      else if (shapeName == "ellipsoid")
      {
        const Mantid::DataObjects::PeakShapeEllipsoid& ellipticalShape = dynamic_cast<const Mantid::DataObjects::PeakShapeEllipsoid&>(shape);
        if (m_radiusType == 0)
        {
          std::vector<double> radii(ellipticalShape.abcRadii());
          radius = *(std::max_element(radii.begin(), radii.end()));
        }
        else if (m_radiusType == 1)
        {
          std::vector<double> radii(ellipticalShape.abcRadiiBackgroundOuter());
          radius = *(std::max_element(radii.begin(), radii.end()));
        }
        else if (m_radiusType == 2)
        {
          std::vector<double> radii(ellipticalShape.abcRadiiBackgroundInner());
          radius = *(std::max_element(radii.begin(), radii.end()));
        }
        else 
        {
          throw std::invalid_argument("The shperical peak shape does not have a radius. \n");
        }
      }
      else if (shapeName == "none")
      {
        radius = m_radiusNoShape;
      }
      else 
      {
        throw std::invalid_argument("An unknown peak shape was registered.");
      }

      // Get position
      switch(coordinateSystem)
      {
        case(Mantid::Kernel::SpecialCoordinateSystem::HKL):
          peaksInfo[i] = std::pair<Mantid::Kernel::V3D, double>(peak->getHKL(), radius);
          break;
        case(Mantid::Kernel::SpecialCoordinateSystem::QLab):
          peaksInfo[i] = std::pair<Mantid::Kernel::V3D, double>(peak->getQLabFrame(), radius);
          break;
        case(Mantid::Kernel::SpecialCoordinateSystem::QSample):
          peaksInfo[i] = std::pair<Mantid::Kernel::V3D, double>(peak->getQSampleFrame(), radius);
          break;
        default:
          throw std::invalid_argument("The special coordinate systems don't match.");
      }
    }

    return peaksInfo;
  }
}
}
#include "MantidVatesAPI/LoadVTK.h"
#include "MantidAPI/FileProperty.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include <boost/make_shared.hpp>
#include <vtkStructuredPointsReader.h>
#include <vtkStructuredPoints.h>


using namespace Mantid::API;
using namespace Mantid::MDEvents;
using namespace Mantid::Geometry;

namespace Mantid
{
  namespace VATES
  {
     DECLARE_ALGORITHM( LoadVTK )

     const std::string LoadVTK::name() const
     {
       return "LoadVTK";
     }

     int LoadVTK::version() const
     {
       return 1;
     }

     const std::string LoadVTK::category() const
     {
       return "MDAlgorithms";
     }

     void LoadVTK::init()
     {
       std::vector<std::string> exts;
       exts.push_back("vtk");
       this->declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts), "VTK file to load");
       declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("OutputWorkspace","", Mantid::Kernel::Direction::Output), "MDHistoWorkspace equivalent of vtkRectilinearInput.");
     }

     void LoadVTK::exec()
     {
       std::string filename = getProperty("Filename");

       auto reader = vtkStructuredPointsReader::New();
       reader->SetFileName(filename.c_str());
       reader->Update();
       vtkStructuredPoints* output = reader->GetOutput();

       int nPoints = output->GetNumberOfPoints();
       int nCellx = output->GetNumberOfCells();

       int dimensions[3];
       output->GetDimensions(dimensions);
       double bounds[6];
       output->ComputeBounds();
       output->GetBounds(bounds);

       auto dimX = boost::make_shared<MDHistoDimension>("X", "X", "", bounds[0], bounds[1], dimensions[0]);
       auto dimY = boost::make_shared<MDHistoDimension>("Y", "Y", "", bounds[2], bounds[3], dimensions[1]);
       auto dimZ = boost::make_shared<MDHistoDimension>("Z", "Z", "", bounds[4], bounds[5], dimensions[2]);
       
       MDHistoWorkspace_sptr outputWS = boost::make_shared<MDHistoWorkspace>(dimX, dimY, dimZ);

       this->setProperty("OutputWorkspace", outputWS);
       
       output->Delete();
       return;
     }
  }
}
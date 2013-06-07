#include "MantidVatesAPI/LoadVTK.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Progress.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidKernel/MandatoryValidator.h"
#include <boost/make_shared.hpp>
#include <vtkStructuredPointsReader.h>
#include <vtkStructuredPoints.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnsignedShortArray.h>
#include "MantidKernel/MultiThreaded.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::MDEvents;
using namespace Mantid::Geometry;

namespace Mantid
{
  namespace VATES
  {
    DECLARE_ALGORITHM( LoadVTK)

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
      this->declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
          "Binary legacy VTK rectilinear structured image file to load.");

      auto manditorySignalArrayName = boost::make_shared<MandatoryValidator<std::string> >();

      this->declareProperty("SignalArrayName", "", manditorySignalArrayName,
          "Cell data array name to import as signal/intesity values in the MD workspace.");
      this->declareProperty("ErrorSQArrayName", "",
          "Cell data array name to import as error squared values in the MD workspace.");

      declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("OutputWorkspace", "", Direction::Output),
          "MDHistoWorkspace equivalent of vtkRectilinearInput.");
    }

    void LoadVTK::exec()
    {
      const std::string filename = getProperty("Filename");
      const std::string signalArrayName = getProperty("SignalArrayName");
      const std::string errorSQArrayName = getProperty("ErrorSQArrayName");

      Progress prog(this, 0, 1, 3);
      prog.report("Loading vtkFile");
      auto reader = vtkStructuredPointsReader::New();
      reader->SetFileName(filename.c_str());
      reader->Update();
      vtkStructuredPoints* output = reader->GetOutput();

      int dimensions[3];
      output->GetDimensions(dimensions);
      double bounds[6];
      output->ComputeBounds();
      output->GetBounds(bounds);

      auto dimX = boost::make_shared<MDHistoDimension>("X", "X", "", bounds[0], bounds[1],
          dimensions[0]);
      auto dimY = boost::make_shared<MDHistoDimension>("Y", "Y", "", bounds[2], bounds[3],
          dimensions[1]);
      auto dimZ = boost::make_shared<MDHistoDimension>("Z", "Z", "", bounds[4], bounds[5],
          dimensions[2]);

      prog.report("Converting to MD Workspace");
      MDHistoWorkspace_sptr outputWS = boost::make_shared<MDHistoWorkspace>(dimX, dimY, dimZ);
      const size_t nPoints = outputWS->getNPoints();
      vtkUnsignedShortArray* signals = vtkUnsignedShortArray::SafeDownCast(
          output->GetPointData()->GetArray(signalArrayName.c_str()));
      if (signals == NULL)
      {
        throw std::invalid_argument("Signal array: " + signalArrayName + " does not exist");
      }

      vtkUnsignedShortArray* errorsSQ = vtkUnsignedShortArray::SafeDownCast(
          output->GetPointData()->GetArray(errorSQArrayName.c_str()));
      if (!errorSQArrayName.empty() && errorsSQ == NULL)
      {
        throw std::invalid_argument("Error squared array: " + errorSQArrayName + " does not exist");
      }

      this->setProperty("OutputWorkspace", outputWS);

      double* destinationSignals = outputWS->getSignalArray();
      double* destinationErrorsSQ = outputWS->getErrorSquaredArray();

      if (errorsSQ == NULL)
      {
        PARALLEL_FOR_NO_WSP_CHECK()
        for (size_t i = 0; i < nPoints; ++i)
        {
          PARALLEL_START_INTERUPT_REGION
          destinationSignals[i] = signals->GetValue(i);
          PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION
     }
     else
     {
       PARALLEL_FOR_NO_WSP_CHECK()
       for (size_t i = 0; i < nPoints; ++i)
       {
         PARALLEL_START_INTERUPT_REGION
         destinationSignals[i] = signals->GetValue(i);
         destinationErrorsSQ[i] = errorsSQ->GetValue(i);
         PARALLEL_END_INTERUPT_REGION
       }
       PARALLEL_CHECK_INTERUPT_REGION
     }

prog.report("Complete");
output->Delete();
return;
}
}
}

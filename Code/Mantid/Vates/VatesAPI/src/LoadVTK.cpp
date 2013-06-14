#include "MantidVatesAPI/LoadVTK.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Progress.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/Memory.h"
#include <boost/make_shared.hpp>
#include <vtkStructuredPointsReader.h>
#include <vtkStructuredPoints.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnsignedShortArray.h>
#include <algorithm>
#include <vtkSmartPointer.h>



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

      this->declareProperty("AdaptiveBinned", false, "What type of output workspace to produce. If selected produces an MDEventWorkspace, otherwise an MDHistoWorkspace is made.");

      auto rangeValidator = boost::make_shared<BoundedValidator<double> >(0, 100);
      this->declareProperty("KeepTopPercent", 25.0, rangeValidator, "Only keep the top percentage of SignalArray values in the range min to max. Allow sparse regions to be ignored. Defaults to 25%.");

      setPropertySettings("KeepTopPercent",
                new EnabledWhenProperty("AdaptiveBinned", IS_NOT_DEFAULT));

      declareProperty(new WorkspaceProperty<IMDWorkspace>("OutputWorkspace", "", Direction::Output),
          "MDHistoWorkspace equivalent of vtkRectilinearInput.");

      declareProperty(new PropertyWithValue<int>("SignalMaximum", 0, Direction::Output), "Maximum signal value determined from input array." );
      declareProperty(new PropertyWithValue<int>("SignalMinimum", 0, Direction::Output), "Minimum signal value determined from input array." );
      declareProperty(new PropertyWithValue<int>("SignalThreshold", 0, Direction::Output), "Actual calculated signal threshold determined from minimum, and maximum signal." );
    }

    void LoadVTK::execMDHisto(vtkUnsignedShortArray* signals, vtkUnsignedShortArray* errorsSQ, MDHistoDimension_sptr dimX, MDHistoDimension_sptr dimY, MDHistoDimension_sptr dimZ, Progress& prog, const int64_t nPoints, const int64_t frequency)
    {
      prog.report("Converting to MD Histogram Workspace");
      MDHistoWorkspace_sptr outputWS = boost::make_shared<MDHistoWorkspace>(dimX, dimY, dimZ);

      double* destinationSignals = outputWS->getSignalArray();
      double* destinationErrorsSQ = outputWS->getErrorSquaredArray();

      if (errorsSQ == NULL)
      {
        PARALLEL_FOR_NO_WSP_CHECK()
        for (int64_t i = 0; i < nPoints; ++i)
        {
          PARALLEL_START_INTERUPT_REGION
          destinationSignals[i] = signals->GetValue(i);
          if(i%frequency == 0)
            prog.report();
          PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION
     }
     else
     {
       PARALLEL_FOR_NO_WSP_CHECK()
       for (int64_t i = 0; i < nPoints; ++i)
       {
         PARALLEL_START_INTERUPT_REGION
         destinationSignals[i] = signals->GetValue(i);
         destinationErrorsSQ[i] = errorsSQ->GetValue(i);
         if(i%frequency == 0)
           prog.report();
         PARALLEL_END_INTERUPT_REGION
       }
       PARALLEL_CHECK_INTERUPT_REGION
     }

     prog.report("Complete");
     this->setProperty("OutputWorkspace", outputWS);
    }

    void LoadVTK::execMDEvent(vtkDataSet* readDataset, vtkUnsignedShortArray* signals, vtkUnsignedShortArray* errorsSQ, MDHistoDimension_sptr dimX, MDHistoDimension_sptr dimY, MDHistoDimension_sptr dimZ, Progress& prog, const int64_t nPoints, const int64_t frequency)
    {
      uint min = std::numeric_limits<uint>::max();
      uint max = std::numeric_limits<uint>::min();
      for (unsigned int i = 0; i < readDataset->GetNumberOfPoints(); ++i)
      {
        uint cv = signals->GetValue(i);
        min = std::min(cv, min);
        max = std::max(cv, max);
      }
      const double keepTopPercent = getProperty("KeepTopPercent");
      const double ditchBottomFraction = (1 - (keepTopPercent/100));
      double m = (max - min);
      double c = min;
      double lowerBounds =  (m * ditchBottomFraction) + c;
      setProperty("SignalMinimum", int(min));
      setProperty("SignalMaximum", int(max));
      setProperty("SignalThreshold", int(lowerBounds));

      std::stringstream ss;
      ss << "Range is Min: " << min << " Max: " << max << std::endl;
      this->g_log.debug(ss.str());
      ss.clear();
      ss << "Signal Limit is: " << lowerBounds;
      this->g_log.debug(ss.str());

      prog.report("Converting to MD Event Workspace");
      auto ws = boost::make_shared<MDEventWorkspace<MDLeanEvent<3>, 3> >();
      auto bc = ws->getBoxController();
      bc->setSplitInto(2);
      bc->setSplitThreshold(10);
      bc->setMaxDepth(7);
      ws->addDimension(dimX);
      ws->addDimension(dimY);
      ws->addDimension(dimZ);
      ws->initialize();

      if(errorsSQ == NULL)
      {
          PARALLEL_FOR1(ws)
          for(int64_t i = 0; i < nPoints; ++i)
          {
            PARALLEL_START_INTERUPT_REGION
            double coordinates[3];
            readDataset->GetPoint(i, coordinates);
            double signal = signals->GetValue(i);

              if(signal > lowerBounds)
              {
                MDLeanEvent<3> event(signal, 0, coordinates);
                ws->addEvent(event);
              }
              if(i%frequency == 0)
                prog.report();
            PARALLEL_END_INTERUPT_REGION
          }
          PARALLEL_CHECK_INTERUPT_REGION
      }
      else
      {
           PARALLEL_FOR1(ws)
           for(int64_t i = 0; i < nPoints; ++i)
           {
             PARALLEL_START_INTERUPT_REGION
             double coordinates[3];
             readDataset->GetPoint(i, coordinates);
             double signal = signals->GetValue(i);
             double errorSQ = errorsSQ->GetValue(i);
               if(signal > lowerBounds)
               {
                 MDLeanEvent<3> event(signal, errorSQ, coordinates);
                 ws->addEvent(event);
               }
               if(i%frequency == 0)
                 prog.report();
             PARALLEL_END_INTERUPT_REGION
           }
           PARALLEL_CHECK_INTERUPT_REGION
      }
      ws->splitBox();
      auto threadScheduler = new Kernel::ThreadSchedulerFIFO();
      Kernel::ThreadPool threadPool(threadScheduler);
      ws->splitAllIfNeeded(threadScheduler);
      threadPool.joinAll();
      ws->refreshCache();
      setProperty("OutputWorkspace", ws);

    }

    void LoadVTK::exec()
    {
      const std::string filename = getProperty("Filename");
      const std::string signalArrayName = getProperty("SignalArrayName");
      const std::string errorSQArrayName = getProperty("ErrorSQArrayName");
      const bool adaptiveBinned = getProperty("AdaptiveBinned");

      Progress prog(this, 0, 1, 102);
      prog.report("Loading vtkFile");
      auto reader = vtkStructuredPointsReader::New();
      reader->SetFileName(filename.c_str());
      reader->Update();

      vtkSmartPointer<vtkStructuredPoints> readDataset;
      readDataset.TakeReference(reader->GetOutput());

      MemoryStats memoryStats;
      const size_t freeMemory = memoryStats.availMem(); // in kB
      const size_t memoryCost = MDHistoWorkspace::sizeOfElement() * readDataset->GetNumberOfPoints() / 1000; // in kB
      if (memoryCost > freeMemory)
      {
      std::string basicMessage = "Loading this file requires more free memory than you have available.";
      std::stringstream sstream;
      sstream << basicMessage << " Requires " << memoryCost << " KB of contiguous memory. You have "
      << freeMemory << " KB.";
      g_log.notice(sstream.str());
      throw std::runtime_error(basicMessage);
      }

      vtkUnsignedShortArray* signals = vtkUnsignedShortArray::SafeDownCast(
          readDataset->GetPointData()->GetArray(signalArrayName.c_str()));
       if (signals == NULL)
       {
         throw std::invalid_argument("Signal array: " + signalArrayName + " does not exist");
       }

       vtkUnsignedShortArray* errorsSQ = vtkUnsignedShortArray::SafeDownCast(
           readDataset->GetPointData()->GetArray(errorSQArrayName.c_str()));
       if (!errorSQArrayName.empty() && errorsSQ == NULL)
       {
         throw std::invalid_argument("Error squared array: " + errorSQArrayName + " does not exist");
       }

      int dimensions[3];
      readDataset->GetDimensions(dimensions);
      double bounds[6];
      readDataset->ComputeBounds();
      readDataset->GetBounds(bounds);

      auto dimX = boost::make_shared<MDHistoDimension>("X", "X", "", bounds[0], bounds[1],
          dimensions[0]);
      auto dimY = boost::make_shared<MDHistoDimension>("Y", "Y", "", bounds[2], bounds[3],
          dimensions[1]);
      auto dimZ = boost::make_shared<MDHistoDimension>("Z", "Z", "", bounds[4], bounds[5],
          dimensions[2]);

      const int64_t nPoints = static_cast<int64_t>( readDataset->GetNumberOfPoints() );
      int frequency = nPoints;
      if(nPoints > 100)
      {
        frequency = nPoints/100;
      }

      if(adaptiveBinned)
      {
        execMDEvent(readDataset, signals, errorsSQ, dimX, dimY, dimZ, prog, nPoints, frequency);
      }
      else
      {
        execMDHisto(signals, errorsSQ, dimX, dimY, dimZ, prog, nPoints, frequency);
      }
}
}
}

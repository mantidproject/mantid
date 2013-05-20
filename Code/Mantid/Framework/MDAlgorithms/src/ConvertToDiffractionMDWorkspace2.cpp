/*WIKI* 

This algorithm converts from a [[MatrixWorkspace]] (in detector/time-of-flight space) to a
[[MDEventWorkspace]] containing events in reciprocal space.

The calculations apply only to elastic diffraction experiments.
The conversion can be done either to Q-space in the lab or sample frame, or to HKL of the crystal.

If the OutputWorkspace does NOT already exist, a default one is created.
In order to define more precisely the parameters of the [[MDEventWorkspace]],
use the [[CreateMDWorkspace]] algorithm first.

==== Types of Conversion ====

* '''Q (lab frame)''': this calculates the momentum transfer (ki-kf) for each event is calculated in the experimental lab frame.
* '''Q (sample frame)''': the goniometer rotation of the sample is taken out, to give Q in the frame of the sample. See [[SetGoniometer]] to specify the goniometer used in the experiment.
* '''HKL''': uses the UB matrix (see [[SetUB]], [[FindUBUsingFFT]] and others) to calculate the HKL Miller indices of each event.

==== Lorentz Correction ====

If selected, the following Lorentz correction factor is applied on each event
by multiplying its weight by L:

<math>L = \frac{ sin(\theta)^2 } { \lambda^{4} } </math>

Where <math>\theta</math> is ''half'' of the neutron scattering angle (conventionally called <math>2\theta</math>).
<math>\lambda</math> is the neutron wavelength in ''Angstroms''.

This correction is also done by the [[AnvredCorrection]] algorithm, and will be set to false if
that algorithm has been run on the input workspace.

==== OneEventPerBin option ====

If you specify ''OneEventPerBin'', then the '''histogram''' representation of the input workspace is used,
with one MDEvent generated for each bin of the workspace, '''including zeros'''.

This can be useful in cases where the experimental coverage needs to be tracked. With one MDEvent for each
bin, you can count which regions in Q-space have been measured. The [[SliceViewer]] has an option
to view normalized by number of events. This means that, for example, areas with overlap from two runs
will appear scaled down.

A significant drawback to this is that the output MDEventWorkspace will be ''significantly'' larger than the
events alone would be. It currently must be created in physical memory (it cannot yet be cached to disk).
One way to limit the memory used is to limit the OutputExtents to a smaller region and only convert
part of the space.

Also, the [[FindPeaksMD]] algorithm may not work optimally because it depends partly on higher density of
events causing more finely split boxes.

If your input is a [[Workspace2D]] and you do NOT check ''OneEventPerBin'', then the workspace is converted
to an [[EventWorkspace]] but with no events for empty bins.

==== Performance Notes ====

* 8-core Intel Xeon 3.2 GHz computer: measured between 4 and 5.5 million events per second (100-200 million event workspace).
* 32-core AMD Opteron 2.7 GHz computer: measured between 8 and 9 million events per second (400-1000 million event workspaces).

*WIKI*/

#include "MantidMDAlgorithms/ConvertToDiffractionMDWorkspace2.h"

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/ProgressText.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidKernel/ListValidator.h"

#include "MantidMDEvents/MDWSTransform.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

namespace Mantid
{
namespace MDAlgorithms
{



  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(ConvertToDiffractionMDWorkspace2)
  
  /// Sets documentation strings for this algorithm
  void ConvertToDiffractionMDWorkspace2::initDocs()
  {
    this->setWikiSummary("Create a MDEventWorkspace with events in reciprocal space (Qx, Qy, Qz) for an elastic diffraction experiment.");
    this->setOptionalMessage("Create a MDEventWorkspace with events in reciprocal space (Qx, Qy, Qz) for an elastic diffraction experiment.");
  }

  
  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void ConvertToDiffractionMDWorkspace2::init()
  {
    // Input units must be TOF
    auto validator = boost::make_shared<API::WorkspaceUnitValidator>("TOF");
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input, validator),
        "An input workspace.");

    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("OutputWorkspace","",Direction::Output),
        "Name of the output MDEventWorkspace. If the workspace already exists, then the events will be added to it.");
    declareProperty(new PropertyWithValue<bool>("Append", false, Direction::Input),
        "Append events to the output workspace. The workspace is replaced if unchecked.");
    // Disabled for this version
    declareProperty(new PropertyWithValue<bool>("ClearInputWorkspace", false, Direction::Input),
        "Clear the events from the input workspace during conversion, to save memory.");
    //TODO --> investigate
    declareProperty(new PropertyWithValue<bool>("OneEventPerBin", false, Direction::Input),
        "Use the histogram representation (event for event workspaces).\n"
        "One MDEvent will be created for each histogram bin (even empty ones).\n"
        "Warning! This can use signficantly more memory!");


    frameOptions.push_back("Q (sample frame)");
    frameOptions.push_back("Q (lab frame)");
    frameOptions.push_back("HKL");
    declareProperty("OutputDimensions", "Q (lab frame)",boost::make_shared<StringListValidator>(frameOptions),
      "What will be the dimensions of the output workspace?\n"
      "  Q (lab frame): Wave-vector change of the lattice in the lab frame.\n"
      "  Q (sample frame): Wave-vector change of the lattice in the frame of the sample (taking out goniometer rotation).\n"
      "  HKL: Use the sample's UB matrix to convert to crystal's HKL indices."
       );

    declareProperty(new PropertyWithValue<bool>("LorentzCorrection", false, Direction::Input),
        "Correct the weights of events with by multiplying by the Lorentz formula: sin(theta)^2 / lambda^4");

    // Box controller properties. These are the defaults
    this->initBoxControllerProps("2" /*SplitInto*/, 1500 /*SplitThreshold*/, 20 /*MaxRecursionDepth*/);

    declareProperty(
      new PropertyWithValue<int>("MinRecursionDepth", 0),
      "Optional. If specified, then all the boxes will be split to this minimum recursion depth. 1 = one level of splitting, etc.\n"
      "Be careful using this since it can quickly create a huge number of boxes = (SplitInto ^ (MinRercursionDepth * NumDimensions)).\n"
      "But setting this property equal to MaxRecursionDepth property is necessary if one wants to generate multiple file based workspaces in order to merge them later\n");
    setPropertyGroup("MinRecursionDepth", getBoxSettingsGroupName());

    std::vector<double> extents(2,0);
    extents[0]=-50;extents[1]=+50;
    declareProperty(
      new ArrayProperty<double>("Extents", extents),
      "A comma separated list of min, max for each dimension,\n"
      "specifying the extents of each dimension. Optional, default +-50 in each dimension.");
    setPropertyGroup("Extents", getBoxSettingsGroupName());
  }

  void ConvertToDiffractionMDWorkspace2::convertFramePropertyNames(const std::string &TargFrame,std::string &TargFrameName,std::string & ScalingName)
  {
    // ----------------- Handle the type of output -------------------------------------

    MDEvents::MDWSTransform QSclAndFrames;

    if (TargFrame == frameOptions[0]) // "Q (sample frame)"
    {
        TargFrameName = QSclAndFrames.getTargetFrame(MDEvents::CnvrtToMD::SampleFrame);
        ScalingName   = QSclAndFrames.getQScaling(MDEvents::CnvrtToMD::NoScaling); //< momentums in A^-1
    }
    else if(TargFrame == frameOptions[1]) //     "Q (lab frame)"
    {
       TargFrameName = QSclAndFrames.getTargetFrame(MDEvents::CnvrtToMD::LabFrame);
       ScalingName   = QSclAndFrames.getQScaling(MDEvents::CnvrtToMD::NoScaling); //< momentums in A^-1
    }
    else if (TargFrame == frameOptions[2]) // "HKL"
    {
       TargFrameName = QSclAndFrames.getTargetFrame(MDEvents::CnvrtToMD::HKLFrame);
       ScalingName   = QSclAndFrames.getQScaling(MDEvents::CnvrtToMD::HKLScale); //< momentums in A^-1
    }
    else
    {
        throw std::invalid_argument("ConvertToDiffractionMDWorkspace2::Unknown target frame: "+TargFrame);
    }

  }

  void ConvertToDiffractionMDWorkspace2::convertExtents(const std::string &Extents,std::string &minVal,std::string &maxVal)
  {
      minVal.resize(3);
      maxVal.resize(3);
      if (Extents.size() == 2)
      {
        for (size_t d=0; d<3; d++)
        {
            minVal[d]=Extents[0];
            maxVal[d]=Extents[1];
        }
      }
      else if(Extents.size() == 6)
      {
        for (size_t d=0; d<3; d++)
        {
            minVal[d]=Extents[2*d+0];
            maxVal[d]=Extents[2*d+1];
        }
      }
      else
        throw std::invalid_argument("You must specify either 2 or 6 extents (min,max).");

  }
  

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.   */
  void ConvertToDiffractionMDWorkspace2::exec()
  {

    // ---------------------- Extract properties --------------------------------------
      //TODO --> Reconsile 
    //OneEventPerBin = getProperty("OneEventPerBin");

    // -------- Input workspace -> convert to Event ------------------------------------

    ////Workspace2D_sptr m_InWS2D = boost::dynamic_pointer_cast<Workspace2D>(m_inWS);
    //if (LorentzCorrection)
    //{
    //  API::Run & run = m_inWS->mutableRun();
    //  if ( run.hasProperty("LorentzCorrection") )
    //  {
    //    Kernel::Property* prop = run.getProperty("LorentzCorrection");
    //    bool lorentzDone = boost::lexical_cast<bool,std::string>(prop->value());
    //    if(lorentzDone)
    //    {
    //      LorentzCorrection = false;
    //      g_log.warning()<<"Lorentz Correction was already done for this workspace.  LorentzCorrection was changed to false." << std::endl;
    //    }
    //  }
    //}
         


    Mantid::API::Algorithm_sptr Convert = createChildAlgorithm("ConvertToMD");
    Convert->initialize();
   
    Convert->setRethrows(true);
    Convert->initialize();
    API::MatrixWorkspace_const_sptr inWS = getProperty("InputWorkspace");
    std::string outWSName = this->getPropertyValue("OutputWorkspace");

    Convert->setProperty("InputWorkspace",inWS);
    Convert->setProperty("OutputWorkspace",outWSName);
    Convert->setProperty("OverwriteExisting",!this->getProperty("Append"));


     if(!MDEvents::MDTransfFactory::Instance().exists("Q3D"))
     {
         throw std::runtime_error(" ConvertToMD Q3D plugin used to transform into DiffractionWorkspaced has not been registered with the MDTransformation factory");
     }
     Convert->setPropertyValue("QDimensions","Q3D");

     std::vector<std::string> dE_modes = Kernel::DeltaEMode().availableTypes();
     Convert->setPropertyValue("dEAnalysisMode",dE_modes[Kernel::DeltaEMode::Elastic]);

     std::string TargetFrame,Scaling;
     this->convertFramePropertyNames(this->getPropertyValue("OutputDimensions"),TargetFrame,Scaling);
     Convert->setProperty("Q3DFrames",TargetFrame);
     Convert->setProperty("QConversionScales",Scaling);

     Convert->setProperty("OtherDimensions","");
     Convert->setProperty("PreprocDetectorsWS","-"); 

     bool lorCorr = this->getProperty("LorentzCorrection");
     Convert->setProperty("LorentzCorrection",lorCorr);
     //set extents
     std::string minVal,maxVal;
     convertExtents(this->getProperty("Extents"),minVal,maxVal);
     Convert->setProperty("MinValues",minVal);
     Convert->setProperty("MaxValues",maxVal);


   // Box controller properties. Has defaults
      Convert->setProperty("SplitInto",this->getPropertyValue("SplitInto"));
      Convert->setProperty("SplitThreshold",this->getPropertyValue("SplitThreshold"));
      Convert->setProperty("MaxRecursionDepth",this->getPropertyValue("MaxRecursionDepth"));
      Convert->setProperty("MinRecursionDepth",this->getPropertyValue("MinRecursionDepth"));

      Convert->execute();

      IMDEventWorkspace_sptr iOut = Convert->getProperty("OutputWorkspace");
      this->setProperty("OutputWorkspace",iOut);

      
 
  }



} // namespace Mantid
} // namespace MDEvents


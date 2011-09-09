#include "MantidMDAlgorithms/ConvertToQ3DdE.h"

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/NumericAxis.h"

#include "MantidKernel/PhysicalConstants.h"

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/ProgressText.h"
#include "MantidAPI/Progress.h"

#include "MantidMDEvents/MDEvent.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"

#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidAPI/WorkspaceValidators.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
//using namespace Mantid::MDEvents;

namespace Mantid
{
namespace MDAlgorithms
{
// logger for loading workspaces  
   Kernel::Logger& ConvertToQ3DdE::convert_log =Kernel::Logger::get("MD-Algorithms");

// the variable describes the locations of the preprocessed detectors, which can be stored and reused it the algorithm runs for more once;
preprocessed_detectors ConvertToQ3DdE::det_loc;
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertToQ3DdE)
  
/// Sets documentation strings for this algorithm
void ConvertToQ3DdE::initDocs()
{
    this->setWikiSummary("Create a MDEventWorkspace with in the reciprocal space of momentums (Qx, Qy, Qz) and the energy transfer dE from an input transformed to energy workspace. If the OutputWorkspace exists, then events are added to it.");
    this->setOptionalMessage("Create a MDEventWorkspace with in the reciprocal space of momentums (Qx, Qy, Qz) and the energy transfer dE from an input transformed to energy workspace. If the OutputWorkspace exists, then events are added to it.");
    this->setWikiDescription(""
        "The algorithm takes data from the transformed to energy matrix workspace, "
        "converts it into reciprocal space and energy transfer, and places the resulting 4D MDEvents into a [[MDEventWorkspace]]."
        "\n\n"
        "If the OutputWorkspace does NOT already exist, a default one is created. In order to define "
        "more precisely the parameters of the [[MDEventWorkspace]], use the "
        "[[CreateMDEventWorkspace]] algorithm first."
        ""
        );
}

//----------------------------------------------------------------------------------------------
/** Constructor
*/
ConvertToQ3DdE::ConvertToQ3DdE()
{
}
    
//----------------------------------------------------------------------------------------------
/** Destructor
 */
ConvertToQ3DdE::~ConvertToQ3DdE()
{
}
/// MDEvent dimensions used 
typedef MDEvents::MDEvent<4> MDE;
/// helper to create empty MDEventWorkspace woth 4 dimensions 
boost::shared_ptr<MDEvents::MDEventWorkspace<MDE,4> > create_empty4DEventWS(const std::string dimensionNames[4],const std::string dimensionUnits[4],
                                                                              const std::vector<double> &dimMin,const std::vector<double> &dimMax)
{

       boost::shared_ptr<MDEvents::MDEventWorkspace<MDE,4> > ws = 
       boost::shared_ptr<MDEvents::MDEventWorkspace<MDE, 4> >(new MDEvents::MDEventWorkspace<MDE, 4>());
    
      // Give all the dimensions
      for (size_t d=0; d<4; d++)
      {
        MDHistoDimension * dim = new MDHistoDimension(dimensionNames[d], dimensionNames[d], dimensionUnits[d], dimMin[d], dimMax[d], 10);
        ws->addDimension(MDHistoDimension_sptr(dim));
      }
      ws->initialize();

      // Build up the box controller
      MDEvents::BoxController_sptr bc = ws->getBoxController();
      bc->setSplitInto(5);
      bc->setSplitThreshold(1500);
      bc->setMaxDepth(20);
      // We always want the box to be split (it will reject bad ones)
      ws->splitBox();
      return ws;
}
//
const double rad2deg = 180.0 / M_PI;
/// helper function to preprocess the detectors directions
void 
ConvertToQ3DdE::process_detectors_positions(const DataObjects::Workspace2D_const_sptr inputWS)
{

    const size_t nHist = inputWS->getNumberHistograms();

    det_loc.det_dir.resize(nHist);
    det_loc.det_id.resize(nHist);
     // Loop over the spectra
   size_t ic(0);
   for (size_t i = 0; i < nHist; i++){

     Geometry::IDetector_const_sptr spDet;
     try{
        spDet= inputWS->getDetector(i);
     }catch(Kernel::Exception::NotFoundError &){
        continue;
     }
 
    // Check that we aren't dealing with monitor...
    if (spDet->isMonitor())continue;   

     det_loc.det_id[ic] = spDet->getID();
    // dist     =  spDet->getDistance(*sample);
     double polar    =  inputWS->detectorTwoTheta(spDet)*rad2deg;
     double azim     =  spDet->getPhi()*rad2deg;    

     double sPhi=sin(polar);
     double ez = cos(polar);
     double ex = sPhi*cos(azim);
     double ey = sPhi*sin(azim);
 
     det_loc.det_dir[ic].setX(ex);
     det_loc.det_dir[ic].setY(ey);
     det_loc.det_dir[ic].setZ(ez);

     ic++;
   }
   // 
   if(ic<nHist){
       det_loc.det_dir.resize(ic);
       det_loc.det_id.resize(ic);
   }
}
//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ConvertToQ3DdE::init()
{
      CompositeWorkspaceValidator<> *ws_valid = new CompositeWorkspaceValidator<>;
      ws_valid->add(new WorkspaceUnitValidator<>("DeltaE"));
      ws_valid->add(new HistogramValidator<>);
      ws_valid->add(new InstrumentValidator<>);


    declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input,ws_valid),
        "An input Matrix Workspace 2D, processed by Convert to energy (homer) algorithm and its x-axis has to be in the units of energy transfer with energy in mev.");
    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("OutputWorkspace","",Direction::Output),
        "Name of the output MDEventWorkspace. If the workspace already exists, then the events will be added to it.");

     BoundedValidator<double> *minEn = new BoundedValidator<double>();
     minEn->setLower(0);
     declareProperty("EnergyInput",100.,minEn,"The value for the incident energy of the neutrons leaving the source (meV)",Direction::Input);

     // this property is mainly for subalgorithms to set-up as they have to identify 
    declareProperty(new PropertyWithValue<bool>("UsePreprocessedDetectors", true, Direction::Input), 
        "Store the part of the detectors transfromation into reciprocal space to save/reuse it later;");

 
}


 

 
  //----------------------------------------------------------------------------------------------
  /* Execute the algorithm.   */
  void ConvertToQ3DdE::exec()
  {
    Timer tim, timtotal;
    CPUTimer cputim, cputimtotal;

     // -------- Input workspace 
    MatrixWorkspace_sptr inMatrixWS = getProperty("InputWorkspace");
    Workspace2D_sptr inWS2D         = boost::dynamic_pointer_cast<Workspace2D>(inMatrixWS);


    // get the energy axis
    NumericAxis *pEnAxis = dynamic_cast<NumericAxis *>(inWS2D->getAxis(0));
    if(!pEnAxis){
       convert_log.error()<<"Can not get proper energy axis from processed workspace\n";
       throw(std::invalid_argument("Input workspace is not propwer converted to energy workspace"));
    }
    std::vector<double> En_boundaries = pEnAxis->createBinBoundaries();
    size_t lastInd = inWS2D->getAxis(0)->length();
    double E_max = En_boundaries[lastInd];
    double E_min = En_boundaries[0];

    // get and check input energy (TODO: should energy be moved into Run?)
    double Ei = getProperty("EnergyInput");
    if (E_max >Ei){
        convert_log.error()<<"Maximal elergy transferred to sample eq "<<E_max<<" and exceeds the input energy "<<Ei<<std::endl;
        throw(std::invalid_argument("Maximal transferred energy exceeds input energy"));
    }
    // the wawe vector of input neutrons;
    double ki=sqrt(Ei/PhysicalConstants::E_mev_toNeutronWavenumberSq);
    // Try to get the output workspace
    IMDEventWorkspace_sptr i_out = getProperty("OutputWorkspace");
    boost::shared_ptr<MDEvents::MDEventWorkspace<MDE,4> > ws  = boost::dynamic_pointer_cast<MDEvents::MDEventWorkspace<MDE,4> >( i_out );

    std::string dimensionNames[4] = {"Q_x", "Q_y", "Q_z","DeltaE"};
    if (ws){
      // Check that existing workspace dimensions make sense with the desired one (using the name)
       for(int i=0;i<4;i++){
        if (ws->getDimension(i)->getName() != dimensionNames[i]){
           convert_log.error()<<"The existing MDEventWorkspace " + ws->getName() + " has different dimensions than were requested! Either give a different name for the output, or change the OutputDimensions parameter.\n";
           throw std::runtime_error("The existing MDEventWorkspace " + ws->getName() + " has different dimensions than were requested! Either give a different name for the output, or change the OutputDimensions parameter.");
        }
       }
    }else{
        std::vector<double> dimMin(4),dimMax(4);
        dimMin[0]=dimMin[1]=dimMin[2]=-50.;
        dimMin[3]=E_min;
        dimMax[0]=dimMax[1]=dimMax[2]= 50.;
        dimMax[3]=E_max;
        std::string dimensionUnits[4] = {"Amgstroms^-1", "Amgstroms^-1", "Amgstroms^-1","meV"};
        ws = create_empty4DEventWS(dimensionNames,dimensionUnits,dimMin,dimMax);
    }
    ws->splitBox();
    //BoxController_sptr bc = ws->getBoxController();
   // if (!bc)
   //   throw std::runtime_error("Output MDEventWorkspace does not have a BoxController!");

    // copy experiment info into 
    ExperimentInfo_sptr ExperimentInfo(inWS2D->cloneExperimentInfo());
    uint16_t runIndex = ws->addExperimentInfo(ExperimentInfo);
    UNUSED_ARG(runIndex); //TODO:

    // Initalize the matrix to 3x3 identity
    mat = Kernel::Matrix<double>(3,3, true);
    // Set the matrix based on UB etc.
    Kernel::Matrix<double> ub = inWS2D->sample().getOrientedLattice().getUB();
    Kernel::Matrix<double> gon =inWS2D->mutableRun().getGoniometerMatrix();
    // As per Busing and Levy 1967, HKL = Goniometer * UB * q_lab_frame
    mat = gon * ub;

    const size_t numSpec  = inWS2D->getNumberHistograms();
    const size_t specSize = inWS2D->blocksize();    

    // Initialise the progress reporting object
    Progress progress(this,0.0,1.0,numSpec);
   // Try to check if one should use preprocessed detector positions or try to calculate the new one
    bool reuse_preprocecced_detectors = getProperty("UsePreprocessedDetectors");
    if(!(reuse_preprocecced_detectors&&det_loc.is_defined()))process_detectors_positions(inWS2D);
 
    // allocate the events buffer;
      std::vector<MDE> out_events;
      out_events.reserve(specSize);

      // Loop over every cell in the workspace, calling the abstract correction function
     // PARALLEL_FOR1(inWS2D)
      for (int64_t i = 0; i < int64_t(numSpec); ++i)
      {
   //     PARALLEL_START_INTERUPT_REGION
        const MantidVec& E_transfer = inWS2D->readX(i);
      //TODO: const MantidVec& Y = inWS2D->readY(i);
      //TODO: const MantidVec& Error = inWS2D->readE(i);
    
        for (size_t j = 0; j < specSize; ++j)
        {
            double E_tr = 0.5*(E_transfer[j]+E_transfer[j+1]);
            double k_tr = sqrt((Ei-E_tr)/PhysicalConstants::E_mev_toNeutronWavenumberSq);
   
            double  ex = det_loc.det_dir[i].X();
            double  ey = det_loc.det_dir[i].Y();
            double  ez = det_loc.det_dir[i].Z();
            double  qz  = ki - ez*k_tr;
            double  qy  = -ey*k_tr;
            double  qx  = -ex*k_tr;
            UNUSED_ARG(qx);
            UNUSED_ARG(qy);
            UNUSED_ARG(qz);

          //performUnaryOperation(XIn,Y[j],E[j],YOut[j],EOut[j]);
        }
        
        progress.report();
 //       PARALLEL_END_INTERUPT_REGION
      }
 //    PARALLEL_CHECK_INTERUPT_REGION
      


    // Save the output
    setProperty("OutputWorkspace", boost::dynamic_pointer_cast<IMDEventWorkspace>(ws));

//
//
//    // ------------------- Cache values that are common for all ---------------------------
//    // Extract some parameters global to the instrument
//    in_ws->getInstrument()->getInstrumentParameters(l1,beamline,beamline_norm, samplePos);
//    beamline_norm = beamline.norm();
//    beamDir = beamline / beamline.norm();
//
//    //To get all the detector ID's
//    in_ws->getInstrument()->getDetectors(allDetectors);
//
//    size_t totalCost = in_ws->getNumberEvents();
//    prog = new Progress(this, 0, 1.0, totalCost);
////    if (DODEBUG) prog = new ProgressText(0, 1.0, totalCost, true);
////    if (DODEBUG) prog->setNotifyStep(1);
//
//    // Create the thread pool that will run all of these.
//    ThreadScheduler * ts = new ThreadSchedulerLargestCost();
//    ThreadPool tp(ts);
//
//    // To track when to split up boxes
//    size_t eventsAdded = 0;
//    size_t lastNumBoxes = ws->getBoxController()->getTotalNumMDBoxes();
//    if (DODEBUG) std::cout << cputim << ": initial setup. There are " << lastNumBoxes << " MDBoxes.\n";
//
//    for (size_t wi=0; wi < in_ws->getNumberHistograms(); wi++)
//    {
//      // Equivalent of: this->convertEventList(wi);
//      EventList & el = in_ws->getEventList(wi);
//
//      // We want to bind to the right templated function, so we have to know the type of TofEvent contained in the EventList.
//      boost::function<void ()> func;
//      switch (el.getEventType())
//      {
//      case TOF:
//        func = boost::bind(&ConvertToQ3DdE::convertEventList<TofEvent>, &*this, static_cast<int>(wi));
//        break;
//      case WEIGHTED:
//        func = boost::bind(&ConvertToQ3DdE::convertEventList<WeightedEvent>, &*this, static_cast<int>(wi));
//        break;
//      case WEIGHTED_NOTIME:
//        func = boost::bind(&ConvertToQ3DdE::convertEventList<WeightedEventNoTime>, &*this, static_cast<int>(wi));
//        break;
//      default:
//        throw std::runtime_error("EventList had an unexpected data type!");
//      }
//
//      // Give this task to the scheduler
//      double cost = double(el.getNumberEvents());
//      ts->push( new FunctionTask( func, cost) );
//
//      // Keep a running total of how many events we've added
//      eventsAdded += el.getNumberEvents();
//      if (bc->shouldSplitBoxes(eventsAdded, lastNumBoxes))
//      {
//        if (DODEBUG) std::cout << cputim << ": Added tasks worth " << eventsAdded << " events.\n";
//        // Do all the adding tasks
//        tp.joinAll();
//        if (DODEBUG) std::cout << cputim << ": Performing the addition of these events.\n";
//
//        // Now do all the splitting tasks
//        ws->splitAllIfNeeded(ts);
//        if (ts->size() > 0)
//          prog->doReport("Splitting Boxes");
//        tp.joinAll();
//
//        // Count the new # of boxes.
//        lastNumBoxes = ws->getBoxController()->getTotalNumMDBoxes();
//        if (DODEBUG) std::cout << cputim << ": Performing the splitting. There are now " << lastNumBoxes << " boxes.\n";
//        eventsAdded = 0;
//      }
//    }
//
//    if (DODEBUG) std::cout << cputim << ": We've added tasks worth " << eventsAdded << " events.\n";
//
//    tp.joinAll();
//    if (DODEBUG) std::cout << cputim << ": Performing the FINAL addition of these events.\n";
//
//    // Do a final splitting of everything
//    ws->splitAllIfNeeded(ts);
//    tp.joinAll();
//    if (DODEBUG) std::cout << cputim << ": Performing the FINAL splitting of boxes. There are now " << ws->getBoxController()->getTotalNumMDBoxes() <<" boxes\n";
//
//
//    // Recount totals at the end.
//    cputim.reset();
//    ws->refreshCache();
//    if (DODEBUG) std::cout << cputim << ": Performing the refreshCache().\n";
//
//    //TODO: Centroid in parallel, maybe?
//    ws->getBox()->refreshCentroid(NULL);
//    if (DODEBUG) std::cout << cputim << ": Performing the refreshCentroid().\n";
//
//
//    if (DODEBUG)
//    {
//      std::cout << "Workspace has " << ws->getNPoints() << " events. This took " << cputimtotal << " in total.\n";
//      std::vector<std::string> stats = ws->getBoxControllerStats();
//      for (size_t i=0; i<stats.size(); ++i)
//        std::cout << stats[i] << "\n";
//      std::cout << std::endl;
//    }
//
//
  }

 ////----------------------------------------------------------------------------------------------
 // /** Convert an event list to 3D q-space and add it to the MDEventWorkspace
 //  *
 //  * @tparam T :: the type of event in the input EventList (TofEvent, WeightedEvent, etc.)
 //  * @param workspaceIndex :: index into the workspace
 //  */
 // template <class T>
 // void ConvertToQ3DdE::convertEventList(int workspaceIndex)
 // {
 //   EventList & el = in_ws->getEventList(workspaceIndex);
 //   size_t numEvents = el.getNumberEvents();

 //   // Get the position of the detector there.
 //   std::set<detid_t>& detectors = el.getDetectorIDs();
 //   if (detectors.size() > 0)
 //   {
 //     // The 3D MDEvents that will be added into the MDEventWorkspce
 //     std::vector<MDE> out_events;
 //     out_events.reserve( el.getNumberEvents() );

 //     // Get the detector (might be a detectorGroup for multiple detectors)
 //     IDetector_const_sptr det = in_ws->getDetector(workspaceIndex);

 //     // Vector between the sample and the detector
 //     V3D detPos = det->getPos() - samplePos;

 //     // Neutron's total travelled distance
 //     double distance = detPos.norm() + l1;

 //     // Detector direction normalized to 1
 //     V3D detDir = detPos / detPos.norm();

 //     // The direction of momentum transfer in the inelastic convention ki-kf
 //     //  = input beam direction (normalized to 1) - output beam direction (normalized to 1)
 //     V3D Q_dir_lab_frame = beamDir - detDir;

 //     // Multiply by the rotation matrix to convert to Q in the sample frame (take out goniometer rotation)
 //     // (or to HKL, if that's what the matrix is)
 //     V3D Q_dir = mat * Q_dir_lab_frame;

 //     // For speed we extract the components.
 //     double Q_dir_x = Q_dir.X();
 //     double Q_dir_y = Q_dir.Y();
 //     double Q_dir_z = Q_dir.Z();

 //     // For lorentz correction, calculate  sin(theta))^2
 //     double sin_theta_squared = 0;
 //     if (LorentzCorrection)
 //     {
 //       // Scattering angle = angle between neutron beam direction and the detector (scattering) direction
 //       double theta = detDir.angle(beamDir);
 //       sin_theta_squared = sin(theta);
 //       sin_theta_squared = sin_theta_squared * sin_theta_squared; // square it
 //     }

 //     /** Constant that you divide by tof (in usec) to get wavenumber in ang^-1 :
 //      * Wavenumber (in ang^-1) =  (PhysicalConstants::NeutronMass * distance) / ((tof (in usec) * 1e-6) * PhysicalConstants::h) * 1e-10; */
 //     const double wavenumber_in_angstrom_times_tof_in_microsec =
 //         (PhysicalConstants::NeutronMass * distance * 1e-10) / (1e-6 * PhysicalConstants::h);

 //     //std::cout << wi << " : " << el.getNumberEvents() << " events. Pos is " << detPos << std::endl;
 //     //std::cout << Q_dir.norm() << " Qdir norm" << std::endl;

 //     // This little dance makes the getting vector of events more general (since you can't overload by return type).
 //     typename std::vector<T> * events_ptr;
 //     getEventsFrom(el, events_ptr);
 //     typename std::vector<T> & events = *events_ptr;

 //     // Iterators to start/end
 //     typename std::vector<T>::iterator it = events.begin();
 //     typename std::vector<T>::iterator it_end = events.end();

 //     for (; it != it_end; it++)
 //     {
 //       // Get the wavenumber in ang^-1 using the previously calculated constant.
 //       double wavenumber = wavenumber_in_angstrom_times_tof_in_microsec / it->tof();

 //       // Q vector = K_final - K_initial = wavenumber * (output_direction - input_direction)
 //       coord_t center[3] = {Q_dir_x * wavenumber, Q_dir_y * wavenumber, Q_dir_z * wavenumber};

 //       if (LorentzCorrection)
 //       {
 //         //double lambda = 1.0/wavenumber;
 //         // (sin(theta))^2 / wavelength^4
 //         float correct = float( sin_theta_squared * wavenumber*wavenumber*wavenumber*wavenumber * sin_theta_squared );
 //         // Push the MDLeanEvent but correct the weight.
 //         out_events.push_back( MDE(float(it->weight()*correct), float(it->errorSquared()*correct*correct), center) );
 //       }
 //       else
 //       {
 //         // Push the MDLeanEvent with the same weight
 //         out_events.push_back( MDE(float(it->weight()), float(it->errorSquared()), center) );
 //       }
 //     }

 //     // Clear out the EventList to save memory
 //     if (ClearInputWorkspace)
 //     {
 //       // Track how much memory you cleared
 //       size_t memoryCleared = el.getMemorySize();
 //       // Clear it now
 //       el.clear();
 //       // For Linux with tcmalloc, make sure memory goes back, if you've cleared 200 Megs
 //       MemoryManager::Instance().releaseFreeMemoryIfAccumulated(memoryCleared, (size_t)2e8);
 //     }

 //     // Add them to the MDEW
 //     ws->addEvents(out_events);
 //   }
 //   prog->reportIncrement(numEvents, "Adding Events");
 // }



} // namespace Mantid
} // namespace MDAlgorithms


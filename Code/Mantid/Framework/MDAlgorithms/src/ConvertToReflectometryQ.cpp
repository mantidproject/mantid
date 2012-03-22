/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidAPI/WorkspaceValidators.h"
#include "MantidMDAlgorithms/ConvertToReflectometryQ.h"
#include "MantidKernel/System.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"

#include <boost/shared_ptr.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid
{
namespace MDAlgorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(ConvertToReflectometryQ)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ConvertToReflectometryQ::ConvertToReflectometryQ()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ConvertToReflectometryQ::~ConvertToReflectometryQ()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string ConvertToReflectometryQ::name() const { return "ConvertToReflectometryQ";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int ConvertToReflectometryQ::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string ConvertToReflectometryQ::category() const { return "General";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void ConvertToReflectometryQ::initDocs()
  {
    this->setWikiSummary("TODO: Enter a quick description of your algorithm.");
    this->setOptionalMessage("TODO: Enter a quick description of your algorithm.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void ConvertToReflectometryQ::init()
  {
    auto validator = boost::make_shared<API::WorkspaceUnitValidator>("TOF");
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input, validator),
        "An input workspace in time-of-flight");

    declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("OutputWorkspace","",Direction::Output), "Output 2D Workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void ConvertToReflectometryQ::exec()
  {
    // -------- Input workspace -> convert to Event ------------------------------------
    MatrixWorkspace_sptr in_ws = getProperty("InputWorkspace");
    
    Mantid::API::WorkspaceFactory().Instance().create("");

    //outWs->addDimension(new MDHistoDimension("", "", "A^(-1)")


    // check the input units
    if (in_ws->getAxis(0)->unit()->unitID() != "TOF")
      throw std::invalid_argument("Input event workspace's X axis must be in TOF units.");

    // Try to get the output workspace
    //IMDEventWorkspace_sptr i_out = getProperty("OutputWorkspace");
    //ws = boost::dynamic_pointer_cast<MDEventWorkspace<MDLeanEvent<3>,3> >( i_out );

    Instrument_const_sptr instrument = in_ws->getInstrument();

    V3D sample = instrument->getSample()->getPos();
    V3D source = instrument->getSource()->getPos();
    V3D beamDir = (sample - source);
    V3D normBeamDir = beamDir / beamDir.norm();
    V3D along = instrument->getReferenceFrame()->vecPointingAlongBeam();
    double thetaIn = beamDir.angle(along);
    double thetaFinal = 0;
    

    //Calculate Theta_f and Theta_i

    const size_t nHistograms = in_ws->getNumberHistograms();
    for(size_t index = 0; index < nHistograms; ++index)
    {
      V3D sink = in_ws->getDetector(index)->getPos();
      V3D detectorDir = (sink - sample);
      double distance = detectorDir.norm() + beamDir.norm();
      V3D normaDetectorDir = detectorDir / detectorDir.norm();
      thetaFinal = detectorDir.angle(along);

      auto counts = in_ws->readY(index);
      auto tofs = in_ws->readX(index);
      auto errors = in_ws->readE(index);
      size_t nTofs = in_ws->isHistogramData() ? (tofs.size() - 1) : tofs.size();

      const double wavenumber_in_angstrom_times_tof_in_microsec =
          (PhysicalConstants::NeutronMass * distance * 1e-10) / (1e-6 * PhysicalConstants::h_bar);

      for(size_t tof = 0; tof < nTofs; ++tof)
      {
        //Calculate wave number
        double t = tofs[tof];
        if(in_ws->isHistogramData())
        {
          t = (t + tofs[tof+1])/2;
        }
        double wavenumber = wavenumber_in_angstrom_times_tof_in_microsec / t;
        double qx = wavenumber *(sin(thetaFinal) + sin(thetaIn));
        double qz = wavenumber *(cos(thetaFinal) - cos(thetaIn));

        double signal = counts[tof];
        double error = errors[tof];

       

      }
    }

  }



} // namespace Mantid
} // namespace MDAlgorithms
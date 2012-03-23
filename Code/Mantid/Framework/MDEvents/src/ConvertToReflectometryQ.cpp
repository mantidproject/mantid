/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidAPI/WorkspaceValidators.h"
#include "MantidMDEvents/ConvertToReflectometryQ.h"
#include "MantidKernel/System.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDHistoWorkspace.h"

#include <boost/shared_ptr.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid
{
namespace MDEvents
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

    declareProperty(new WorkspaceProperty<IMDWorkspace>("OutputWorkspace","",Direction::Output), "Output 2D Workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void ConvertToReflectometryQ::exec()
  {

    //TODO rebin input?

    MatrixWorkspace_sptr in_ws = getProperty("InputWorkspace");
    
    //TODO, how to calculate max, min + number of bins? Jon Taylor thinks that there is a way to calculate these upfront.
    const double qzmin = -0.01;
    const double qxmin = -0.01;
    const double qxmax = 0.01;
    const double qzmax = 0.01;
    const size_t nbinsx = 10;
    const size_t nbinsz = 10;

    // Fixed dimensionality
    MDHistoDimension_sptr qxDim = MDHistoDimension_sptr(new MDHistoDimension("Qx","qx","(Ang^-1)", qxmin, qxmax, nbinsx)); 
    MDHistoDimension_sptr qzDim = MDHistoDimension_sptr(new MDHistoDimension("Qz","qz","(Ang^-1)", qzmin, qzmax, nbinsz)); 
    MDHistoWorkspace_sptr ws = MDHistoWorkspace_sptr(new MDHistoWorkspace(qxDim, qzDim));

    // check the input units
    if (in_ws->getAxis(0)->unit()->unitID() != "TOF")
      throw std::invalid_argument("Input event workspace's X axis must be in TOF units.");

    // Calculate theta incident 
    Instrument_const_sptr instrument = in_ws->getInstrument();
    V3D sample = instrument->getSample()->getPos();
    V3D source = instrument->getSource()->getPos();
    V3D beamDir = (sample - source);
    V3D normBeamDir = beamDir / beamDir.norm();
    V3D along = instrument->getReferenceFrame()->vecPointingAlongBeam();
    V3D up = instrument->getReferenceFrame()->vecPointingUp();
    double thetaIn = beamDir.angle(along);
    double thetaFinal = 0;
    
    const size_t nHistograms = in_ws->getNumberHistograms();
    //Loop through all spectra
    for(size_t index = 0; index < nHistograms; ++index)
    {
      V3D sink = in_ws->getDetector(index)->getPos();
      V3D detectorDir = (sink - sample);
      double distance = detectorDir.norm() + beamDir.norm();
      V3D normalDetectorDir = detectorDir / detectorDir.norm();
      thetaFinal = detectorDir.angle(along);

      auto counts = in_ws->readY(index);
      auto tofs = in_ws->readX(index);
      auto errors = in_ws->readE(index);

      size_t nTofs = in_ws->isHistogramData() ? (tofs.size() - 1) : tofs.size();

      const double wavenumber_in_angstrom_times_tof_in_microsec =
          (PhysicalConstants::NeutronMass * distance * 1e-10) / (1e-6 * PhysicalConstants::h_bar);

      //Loop through all TOF
      for(size_t tof = 0; tof < nTofs; ++tof)
      {
        //Calculate wave number
        double t = tofs[tof];
        if(in_ws->isHistogramData())
        {
          t = (t + tofs[tof+1])/2;
        }
        double wavenumber = wavenumber_in_angstrom_times_tof_in_microsec / t;
        
        //ki - kf 
        V3D QDir = (normBeamDir - normalDetectorDir);
        double _qx = QDir.X() * wavenumber;
        double _qz = QDir.Z() * wavenumber;

        /// If q-max and min are known a-prori, these boundrary case truncations are not required. See top of method for comment.
        _qx = _qx < qxmin ? qxmin : _qx;
        _qz = _qz < qzmin ? qzmin : _qz;
        _qx = _qx > qxmax ? qxmax : _qx;
        _qz = _qz > qzmax ? qzmax : _qz;

        //Set up for linear transformation qi -> dimension index.
        double mx = (nbinsx / (qxmax - qxmin));
        double mz = (nbinsz / (qzmax - qzmin));
        double cx = (nbinsx - mx * (qxmin + qxmax))/2;
        double cz = (nbinsz - mz * (qzmin + qzmax))/2;

        double posIndexX = mx*_qx + cx;
        double posIndexZ = mz*_qz + cz;

        size_t linearindex = (posIndexX * nbinsx) + posIndexZ;
        
        double error = errors[tof];
        
        //Do we want to accumulate signal values, i.e will there be any overlap in Q?
        ws->setSignalAt(linearindex, ws->getSignalAt(linearindex) + counts[tof]);
        ws->setErrorSquaredAt(linearindex, error*error);
      }
    }

    setProperty("OutputWorkspace", ws);
  }



} // namespace Mantid
} // namespace MDAlgorithms
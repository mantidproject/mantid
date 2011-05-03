#include "MantidCrystal/PredictPeaks.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/Math/Matrix.h"
#include "MantidGeometry/Objects/InstrumentRayTracer.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/System.h"
#include <cmath>

namespace Mantid
{
namespace Crystal
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(PredictPeaks)
  
  using namespace Mantid::API;
  using namespace Mantid::DataObjects;
  using namespace Mantid::Geometry;
  using namespace Mantid::Kernel;


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  PredictPeaks::PredictPeaks()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  PredictPeaks::~PredictPeaks()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void PredictPeaks::initDocs()
  {
    this->setWikiSummary("Using a known crystal lattice and UB matrix, predict where single crystal peaks should be found in detector/TOF space. Creates a PeaksWorkspace containing the peaks at the expected positions.");
    this->setOptionalMessage("Using a known crystal lattice and UB matrix, predict where single crystal peaks should be found in detector/TOF space. Creates a PeaksWorkspace containing the peaks at the expected positions.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void PredictPeaks::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input),
        "An input workspace containing:\n"
        "  - The relevant Instrument (calibrated as needed).\n"
        "  - A sample with a UB matrix.\n"
        "  - The goniometer rotation matrix.");

    declareProperty(new PropertyWithValue<double>("WavelengthMin",0.1,Direction::Input),
        "Minimum wavelength limit at which to start looking for single-crystal peaks.");
    declareProperty(new PropertyWithValue<double>("WavelengthMax",100.0,Direction::Input),
        "Maximum wavelength limit at which to stop looking for single-crystal peaks.");

    declareProperty(new WorkspaceProperty<PeaksWorkspace>("OutputWorkspace","",Direction::Output), "An output PeaksWorkspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void PredictPeaks::exec()
  {
    // Get the input properties
    MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
    double wlMin = getProperty("WavelengthMin");
    double wlMax = getProperty("WavelengthMax");

    // Check the values.
    if (!inWS) throw std::invalid_argument("Did not specify a valid InputWorkspace.");
    if (wlMin >= wlMax) throw std::invalid_argument("WavelengthMin must be < WavelengthMax.");
    if (wlMin < 1e-5) throw std::invalid_argument("WavelengthMin must be stricly positive.");

    // Get the instrument and its detectors
    IInstrument_sptr inst;
    inst = inWS->getInstrument();
    if (!inst) throw std::invalid_argument("No instrument found in input workspace!");

    // Create the output
    PeaksWorkspace_sptr pw(new PeaksWorkspace());
    setProperty<PeaksWorkspace_sptr>("OutputWorkspace", pw);

    // Get the UB matrix
    Matrix<double> ub(3,3, true);
    ub = ub * 0.05;

    // TODO: Retrieve UB from the workspace
    // TODO: Retrieve the goniometer rotation matrix.
    Matrix<double> gonio(3,3, true);

    // Final transformation matrix (Q in lab frame to HKL)
    Matrix<double> mat = gonio * ub;

    // Sample position
    V3D samplePos = inst->getSample()->getPos();

    // L1 path and direction
    V3D beamDir = inst->getSource()->getPos() - samplePos;
    double L1 = beamDir.normalize(); // Normalize to unity

    if ((fabs(beamDir.X()) > 1e-2) || (fabs(beamDir.Y()) > 1e-2)) // || (beamDir.Z() < 0))
        throw std::invalid_argument("Instrument must have a beam direction that is only in the +Z direction for this algorithm to be valid..");

    // TODO: Determine which HKL to look for
    std::vector<V3D> hkls;
    for (double h=-10; h < 10; h++)
      for (double k=-10; k < 10; k++)
        for (double l=-10; l < 10; l++)
          hkls.push_back(V3D(h,k,l));

    // PARALLEL!!!
    for (int i=0; i < static_cast<int>(hkls.size()); ++i)
    {
      V3D hkl = hkls[i];

      // The q-vector direction of the peak is = goniometer * ub * hkl_vector
      V3D q = mat * hkl;

      // The incident wavevector is in the +Z direction, ki = 1/wl (in z direction)
      // For elastic diffraction, kf - ki = q, therefore:
      // kf = qx in x; qy in y; and (qz+2pi/wl) in z.
      // AND: norm(kf) = norm(ki) = 1.0/wavelength
      // THEREFORE: 1/wl = - norm(q)^2 / (2*qz)

      double norm_q = q.norm();
      double one_over_wl = -(norm_q*norm_q) / (2.0 * q.Z());
      double wl = 1.0/one_over_wl;

      // Only keep going for accepted wavelengths.
      if (wl >= wlMin && wl <= wlMax)
      {
        // This is the scattered direction, kf = ki but with kf_z = (qz+1/wl)
        V3D beam = q;
        beam.setZ(q.Z() + one_over_wl);

        std::cout << hkl << ", q = " << q << "; beam = " << beam << "; wl = " << wl << "\n";

        // Create a ray tracer
        InstrumentRayTracer tracker(inst);
        // Find intersecting instrument components in this direction.
        V3D beamNormalized = beam;
        beamNormalized.normalize();
        //tracker.traceFromSample(beamNormalized);
        Links results = tracker.getResults();

        // Go through all results
        Links::const_iterator resultItr = results.begin();
        for (; resultItr != results.end(); resultItr++)
        {
          IComponent_sptr component = inst->getComponentByID(resultItr->componentID);
          IDetector_sptr det = boost::dynamic_pointer_cast<IDetector>(component);
          if (det)
          {
            if (!det->isMonitor())
            {
              // Found a detector (not a monitor) that intersected the ray. Take the first one!
              std::cout << "HKL " << hkl << " and q " << q << " will project on id " << det->getID() << " at wl " << 1.0/one_over_wl << "\n";

              // Create the peak
              Peak p(inst, det->getID(), wl);
              p.setHKL(hkl);

              // Add it to the workspace
              pw->addPeak(p);

              break;
            }
          }
        }
      }

    }

//    2pi/wl = - norm(q)^2 / (2*qz)
//So this goes into the kf_z = (qz+2pi/wl)
//"""
//#Calcualte the scattered q-vector.
//matrix = np.dot(rot_matrix, ub_matrix)
//q_vector = np.dot(matrix, hkl)
//squared_norm_of_q = np.sum( q_vector**2, axis=0)
//# 2pi/wl = - norm(q)^2 / (2*qz)
//two_pi_over_wl = -0.5 * squared_norm_of_q / q_vector[2,:]
//#Calculate the scattered beam direction
//beam = q_vector.copy()
//#Add 2pi/wl to the Z component only
//beam[2,:] += two_pi_over_wl
//#if the qz is positive, then that scattering does not happen.
//#   set those to nan.
//beam[2, (q_vector[2,:]>0) ] = np.nan
//#And return it.
//return beam

  }



  /*


    std::vector<int> detIDs = inst->getDetectorIDs(true);
    if (detIDs.size() <= 0) throw std::invalid_argument("No detectors found in the instrument!");

    // Go through each detector
    // PARALLEL!!!
    for (int i=0; i < static_cast<int>(detIDs.size()); ++i)
    {
      int id = detIDs[i];
      IDetector_sptr det = inst->getDetector(id);
      if (det)
      {
        // Vector between the sample and the detector
        V3D detPos = det->getPos() - samplePos;

        // Neutron's total travelled distance
        double distance = detPos.norm() + L1;

        // Detector direction normalized to 1
        V3D detDir = detPos / detPos.norm();

        // The direction of momentum transfer = the output beam direction - input beam direction (normalized)
        V3D Q_dir_lab_frame = detDir - beamDir;

        // The min Q = the one corresponding to the longest wavelength
        V3D qMin = Q_dir_lab_frame / wlMax;
        // Conversely for the largest q
        V3D qMax = Q_dir_lab_frame / wlMin;

        // Multiply by the rotation matrix to convert to the HKL start/end points
        V3D hklMin = mat * qMin;
        V3D hklMax = mat * qMax;
        V3D hklDir = hklMax - hklMin;

        std::cout << id << " : " << hklMin << " to " << hklMax << " \n";

        // How many HKL units to step through
        double hklDist = hklDir.normalize();
        size_t numSteps = size_t(ceil(hklDist)) * 20;
        std::vector<V3D> uniqueHKLs;
        double last_diff = 0;

        for (size_t j=0; j<numSteps; ++j)
        {
          V3D hkl = hklMin + (hklDir * (double(j) * 0.05));
          V3D hklRound = hkl;
          hklRound.round();
          double diff = hklRound.norm();
          if (diff > last_diff)
          {
            // The difference is starting to increase
            uniqueHKLs.push_back(hklRound);
            std::cout << hklRound << ", ";
          }
        }

        std::cout << "\n";


      }

    } // (for each detector ID)


   *
   */

} // namespace Mantid
} // namespace Crystal


#include "MantidCrystal/PredictPeaks.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/UnitCell.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/Math/Matrix.h"
#include "MantidGeometry/Objects/InstrumentRayTracer.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/System.h"
#include <cmath>
#include <MantidGeometry/Crystal/OrientedLattice.h>
#include "MantidKernel/ListValidator.h"
#include "MantidGeometry/Crystal/ReflectionCondition.h"

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
    m_refConds = getAllReflectionConditions();
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
    this->setWikiDescription(
        "This algorithm uses the InputWorkspace to determine the instrument in use, as well as the UB Matrix and Unit Cell of the sample used."
        "\n\n"
        "The algorithm operates by calculating the scattering direction (given the UB matrix) for a particular HKL, "
        "and determining whether that hits a detector. The MinDSpacing parameter is used to determine what HKL's to try."
        "\n\n"
        "The parameters of WavelengthMin/WavelengthMax also limit the peaks attempted to those that can be detected/produced by your instrument."
    );
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

    declareProperty(new PropertyWithValue<double>("MinDSpacing",1.0,Direction::Input),
        "Minimum d-spacing of peaks to consider.");

    // Build up a list of reflection conditions to use
    std::vector<std::string> propOptions;
    for (size_t i=0; i<m_refConds.size(); ++i)
      propOptions.push_back( m_refConds[i]->getName() );
    declareProperty("ReflectionCondition", "Primitive",new ListValidator(propOptions),
      "Which reflection condition applies to this crystal, reducing the number of expected HKL peaks?");


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
    double minD = getProperty("MinDSpacing");

    // Check the values.
    if (!inWS) throw std::invalid_argument("Did not specify a valid InputWorkspace.");
    if (wlMin >= wlMax) throw std::invalid_argument("WavelengthMin must be < WavelengthMax.");
    if (wlMin < 1e-5) throw std::invalid_argument("WavelengthMin must be stricly positive.");
    if (minD < 1e-4) throw std::invalid_argument("MinDSpacing must be stricly positive.");

    // Get the instrument and its detectors
    IInstrument_sptr inst;
    inst = inWS->getInstrument();
    if (!inst) throw std::invalid_argument("No instrument found in input workspace!");

    // --- Reflection condition ----
    // Use the primitive by default
    ReflectionCondition_sptr refCond(new ReflectionConditionPrimitive());
    // Get it from the property
    std::string refCondName = getPropertyValue("ReflectionCondition");
    for (size_t i=0; i<m_refConds.size(); ++i)
      if (m_refConds[i]->getName() == refCondName)
        refCond = m_refConds[i];


    // Create the output
    PeaksWorkspace_sptr pw(new PeaksWorkspace());
    setProperty<PeaksWorkspace_sptr>("OutputWorkspace", pw);


    // Retrieve the OrientedLattice (UnitCell) from the workspace
    OrientedLattice crystal = inWS->sample().getOrientedLattice();

    // Get the UB matrix from it
    Matrix<double> ub(3,3, true);
    ub = crystal.getUB();

    // Retrieve the goniometer rotation matrix
    Matrix<double> gonio(3,3, true);
    gonio = inWS->mutableRun().getGoniometerMatrix();

    // Final transformation matrix (Q in lab frame to HKL)
    Matrix<double> mat = gonio * ub;

    // Sample position
    V3D samplePos = inst->getSample()->getPos();

    // L1 path and direction
    V3D beamDir = inst->getSource()->getPos() - samplePos;
    //double L1 = beamDir.normalize(); // Normalize to unity

    if ((fabs(beamDir.X()) > 1e-2) || (fabs(beamDir.Y()) > 1e-2)) // || (beamDir.Z() < 0))
        throw std::invalid_argument("Instrument must have a beam direction that is only in the +Z direction for this algorithm to be valid..");

    // Determine which HKL to look for
    // Inverse d-spacing that is the limit to look for.
    double dstar = 1.0/minD;
    V3D hklMin(0,0,0);
    V3D hklMax(0,0,0);
    for (double qx=-1; qx < 2; qx += 2)
      for (double qy=-1; qy < 2; qy += 2)
        for (double qz=-1; qz < 2; qz += 2)
        {
          // Build a q-vector for this corner of a cube
          V3D Q(qx,qy,qz);
          Q *= dstar;
          V3D hkl = crystal.hklFromQ(Q);
          // Find the limits of each hkl
          for (size_t i=0; i<3; i++)
          {
            if (hkl[i] < hklMin[i]) hklMin[i] = hkl[i];
            if (hkl[i] > hklMax[i]) hklMax[i] = hkl[i];
          }
        }
    // Round to nearest int
    hklMin.round();
    hklMax.round();

    // How many HKLs is that total?
    V3D hklDiff = hklMax-hklMin + V3D(1,1,1);
    size_t numHKLs = size_t( hklDiff[0] * hklDiff[1] * hklDiff[2]);

    g_log.information() << "HKL range for d_min of " << minD << " is from " << hklMin << " to " << hklMax << ", a total of " << numHKLs << " possible HKL's\n";

    if (numHKLs > 10000000000)
      throw std::invalid_argument("More than 10 billion HKLs to search. Is your d_min value too small?");


    // Counter of possible peaks
    size_t numInRange = 0;

    Progress prog(this, 0.0, 1.0, numHKLs);
    prog.setNotifyStep(0.01);

    PRAGMA_OMP(parallel for schedule(dynamic, 1) )
    for (int h=(int)hklMin[0]; h <= (int)hklMax[0]; h++)
    {
      PARALLEL_START_INTERUPT_REGION
      for (int k=(int)hklMin[1]; k <= (int)hklMax[1]; k++)
      {
        for (int l=(int)hklMin[2]; l <= (int)hklMax[2]; l++)
        {
          if (refCond->isAllowed(h,k,l) && (h != 0) && (k != 0) && (l != 0))
          {
            V3D hkl(h,k,l);

            // Skip those with unacceptable d-spacings
            double d = crystal.d(hkl);
            if (d > minD)
            {
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

                //          std::cout << hkl << ", q = " << q << "; beam = " << beam << "; wl = " << wl << "\n";
                PARALLEL_CRITICAL(PredictPeaks_numInRange)
                { numInRange++;
                }

                // Create a ray tracer
                InstrumentRayTracer tracker(inst);
                // Find intersecting instrument components in this direction.
                V3D beamNormalized = beam;
                beamNormalized.normalize();
                tracker.traceFromSample(beamNormalized);
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
                      //                std::cout << "HKL " << hkl << " and q " << q << " will project on id " << det->getID() << " at wl " << 1.0/one_over_wl << "\n";

                      // Create the peak
                      Peak p(inst, det->getID(), wl);
                      p.setHKL(hkl);

                      // Add it to the workspace
                      PARALLEL_CRITICAL(PredictPeaks_appendPeak)
                      {
                        pw->addPeak(p);
                      }

                      break;
                    }
                  } // (is a detector)
                } // each ray tracer result
              } // (wavelength is okay)
            } // (d is acceptable)
          } // refl is allowed and not 0,0,0

          prog.report();
        } // for each l
      } // for each k
      PARALLEL_END_INTERUPT_REGION
    } // for each h
    PARALLEL_CHECK_INTERUPT_REGION

    g_log.information() << "Out of " << numInRange << " allowed peaks within parameters, " << pw->getNumberPeaks() << " were found to hit a detector.\n";
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


#include "MantidMDAlgorithms/ConvertCWSDMDtoHKL.h"

#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDIterator.h"

#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/BoundedValidator.h"

#include "MantidDataObjects/MDEventFactory.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidGeometry/Instrument/ComponentHelper.h"
#include "MantidMDAlgorithms/MDWSDescription.h"
#include "MantidMDAlgorithms/MDWSTransform.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/MDBoxBase.h"
#include "MantidDataObjects/MDEventInserter.h"

namespace Mantid {
namespace MDAlgorithms {

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

DECLARE_ALGORITHM(ConvertCWSDMDtoHKL)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ConvertCWSDMDtoHKL::ConvertCWSDMDtoHKL() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ConvertCWSDMDtoHKL::~ConvertCWSDMDtoHKL() {}

void ConvertCWSDMDtoHKL::init() {
  declareProperty(new WorkspaceProperty<IMDEventWorkspace>("InputWorkspace", "",
                                                           Direction::Input),
                  "Name of the input MDEventWorkspace that stores detectors "
                  "counts from a constant-wave powder diffraction experiment.");

  this->declareProperty(new WorkspaceProperty<PeaksWorkspace>(
                            "PeaksWorkspace", "", Direction::InOut),
                        "Input Peaks Workspace");

  boost::shared_ptr<BoundedValidator<double> > mustBePositive(
      new BoundedValidator<double>());
  mustBePositive->setLower(0.0);
  this->declareProperty(new PropertyWithValue<double>("Tolerance", 0.15,
                                                      mustBePositive,
                                                      Direction::Input),
                        "Indexing Tolerance (0.15)");

  declareProperty(new WorkspaceProperty<IMDEventWorkspace>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name of the output MDEventWorkspace in HKL-space.");
}

/**
  */
void ConvertCWSDMDtoHKL::exec() {

  IMDEventWorkspace_sptr inputWS = getProperty("InputWorkspace");
  // 1. Check the units of the MDEvents
  // 2. Export all the events to text file
  // 3. Get a UB matrix
  // 4. Refer to IndexPeak to calculate H,K,L of each MDEvent
}

std::vector<std::vector<double> >
ConvertCWSDMDtoHKL::exportEvents(IMDEventWorkspace_sptr mdws) {
  size_t numevents = mdws->getNEvents();

  std::vector<std::vector<double> > vec_md_events(numevents);

  IMDIterator *mditer = mdws->createIterator();
  bool scancell = true;
  size_t nextindex = 1;
  int currindex = 0;
  while (scancell) {
    size_t numevent_cell = mditer->getNumEvents();
    for (size_t iev = 0; iev < numevent_cell; ++iev) {
      double tempx = mditer->getInnerPosition(iev, 0);
      double tempy = mditer->getInnerPosition(iev, 1);
      double tempz = mditer->getInnerPosition(iev, 2);
      std::vector<double> v = { 1, 2, 3, 4 };
      vec_md_events[currindex] = v;

      ++currindex;
    }

    return vec_md_events;
  }
}

void ConvertCWSDMDtoHKL::indexQSample(IMDEventWorkspace_sptr mdws,
                                      PeaksWorkspace_sptr peakws) {
  std::vector<V3D> miller_indices;
  std::vector<V3D> q_vectors;

  OrientedLattice o_lattice = peakws->mutableSample().getOrientedLattice();
  Matrix<double> UB = o_lattice.getUB();
  Matrix<double> tempUB(UB);

  int num_indexed = 0;
  int original_indexed = 0;
  double original_error = 0;
  double tolerance = this->getProperty("Tolerance");
  original_indexed = IndexingUtils::CalculateMillerIndices(
      tempUB, q_vectors, tolerance, miller_indices, original_error);
}

//----------------------------------------------------------------------------------------------
/** Create output workspace
 * @brief ConvertCWSDExpToMomentum::createExperimentMDWorkspace
 * @return
 */
API::IMDEventWorkspace_sptr ConvertCWSDMDtoHKL::createHKLMDWorkspace() {
  // Get detector list from input table workspace

  // Create workspace in Q_sample with dimenion as 3
  size_t nDimension = 3;
  IMDEventWorkspace_sptr mdws =
      MDEventFactory::CreateMDWorkspace(nDimension, "MDEvent");

  // Extract Dimensions and add to the output workspace.
  std::vector<std::string> vec_ID(3);
  vec_ID[0] = "H";
  vec_ID[1] = "K";
  vec_ID[2] = "L";

  std::vector<std::string> dimensionNames(3);
  dimensionNames[0] = "H";
  dimensionNames[1] = "K";
  dimensionNames[2] = "L";

  std::vector<double> m_extentMins;
  std::vector<double> m_extentMaxs;
  std::vector<size_t> m_numBins;

  Mantid::Kernel::SpecialCoordinateSystem coordinateSystem =
      Mantid::Kernel::HKL;

  // Add dimensions
  // FIXME - Should I give out a better value???
  if (m_extentMins.size() != 3 || m_extentMaxs.size() != 3 ||
      m_numBins.size() != 3) {
    // Default dimenion
    m_extentMins.resize(3, -10.0);
    m_extentMaxs.resize(3, 10.0);
    m_numBins.resize(3, 100);
  }
  for (size_t d = 0; d < 3; ++d)
    g_log.debug() << "Direction " << d << ", Range = " << m_extentMins[d]
                  << ", " << m_extentMaxs[d] << "\n";

  for (size_t i = 0; i < nDimension; ++i) {
    std::string id = vec_ID[i];
    std::string name = dimensionNames[i];
    // std::string units = "A^-1";
    std::string units = "";
    mdws->addDimension(
        Geometry::MDHistoDimension_sptr(new Geometry::MDHistoDimension(
            id, name, units, static_cast<coord_t>(m_extentMins[i]),
            static_cast<coord_t>(m_extentMaxs[i]), m_numBins[i])));
  }

  // Set coordinate system
  mdws->setCoordinateSystem(coordinateSystem);

  return mdws;
}

void ConvertCWSDMDtoHKL::addMDEvents(
    std::vector<std::vector<Mantid::coord_t> > &vec_q_sample,
    std::vector<double> vec_signal, PeaksWorkspace_sptr ubpeakws) {
  // Create transformation matrix from which the transformation is

  g_log.information() << "Before insert new event, output workspace has "
                      << m_outputWS->getNEvents() << "Events.\n";

  // Creates a new instance of the MDEventInserter to output workspace
  MDEventWorkspace<MDEvent<3>, 3>::sptr mdws_mdevt_3 =
      boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<3>, 3> >(m_outputWS);
  MDEventInserter<MDEventWorkspace<MDEvent<3>, 3>::sptr> inserter(mdws_mdevt_3);

  // Go though each spectrum to conver to MDEvent
  for (size_t iq = 0; iq < vec_q_sample.size(); ++iq) {

    std::vector<Mantid::coord_t> q_sample = vec_q_sample[iq];
    float signal = vec_signal[iq];
    float error = std::sqrt(signal);
    uint16_t runnumber = 1;
    detid_t detid = 1;

    // Insert
    inserter.insertMDEvent(
        static_cast<float>(signal), static_cast<float>(error * error),
        static_cast<uint16_t>(runnumber), detid, q_sample.data());
  }

  return;
}

} // namespace MDAlgorithms
} // namespace Mantid

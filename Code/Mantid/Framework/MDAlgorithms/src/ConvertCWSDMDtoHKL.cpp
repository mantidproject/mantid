#include "MantidMDAlgorithms/ConvertCWSDMDtoHKL.h"

#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDIterator.h"

#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

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

  declareProperty(
      new WorkspaceProperty<IMDEventWorkspace>("OutputWorkspace", "",
                                               Direction::Output),
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

std::vector<std::vector<double> > ConvertCWSDMDtoHKL::exportEvents(IMDEventWorkspace_sptr mdws) {
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

void ConvertCWSDMDtoHKL::indexQSample(IMDEventWorkspace_sptr mdws, PeaksWorkspace_sptr peakws)
{
  std::vector<V3D> miller_indices;
  std::vector<V3D> q_vectors;

  OrientedLattice o_lattice = peakws->mutableSample().getOrientedLattice();
  Matrix<double> UB = o_lattice.getUB();
  Matrix<double> tempUB(UB);

  int num_indexed = 0;
  int original_indexed = 0;
  double original_error = 0;
  original_indexed = IndexingUtils::CalculateMillerIndices(
      tempUB, q_vectors, tolerance, miller_indices, original_error);

}

} // namespace MDAlgorithms
} // namespace Mantid

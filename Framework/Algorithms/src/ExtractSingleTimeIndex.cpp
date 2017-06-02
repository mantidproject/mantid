#include "MantidAlgorithms/ExtractSingleTimeIndex.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidGeometry/Instrument.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/make_unique.h"
#include "MantidKernel/Property.h"
#include "MantidTypes/SpectrumDefinition.h"

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(ExtractSingleTimeIndex)

using namespace API;
using namespace Kernel;

void ExtractSingleTimeIndex::init() {
  declareProperty(
      make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
      "Workspace containing the input data");

  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "The name to give the output workspace");
  auto validator = boost::make_shared<BoundedValidator<int>>();
  validator->setLower(0);
  declareProperty("TimeIndex", EMPTY_INT(), validator,
                  "Load single time index. Only applies to D2B!");
}

void ExtractSingleTimeIndex::exec() {
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  int timeIndex = getProperty("TimeIndex");
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");

  const auto &instrument = inputWS->getInstrument();
  const auto numDet = instrument->getNumberDetectors();

  outputWS = WorkspaceFactory::Instance().create("Workspace2D", numDet, 2, 1);

  const auto &indexInfo = inputWS->indexInfo();
  const auto &spectrumDefs = *(indexInfo.spectrumDefinitions());

  for (size_t i = 0; i < numDet; ++i) {
    auto hist = inputWS->histogram(0);

    for (auto spectraDef : spectrumDefs) {
      for (auto item : spectraDef) {
        if (item.first == i && item.second == timeIndex) {
          hist = inputWS->histogram(i);
        }
      }
    }

    outputWS->setHistogram(i, hist);
  }
}

} // namespace Algorithms
} // namespace Mantid

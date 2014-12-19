/**
 * This algorithm flattens a MDHistoWorkspace to a Workspace2D. Mantid has far
 *more tools
 * to deal with W2D then for MD ones.
 *
 * copyright: do not bother me, see Mantid copyright
 *
 * Mark Koennecke, November 2012
 *
 * Added copying of meta data. Mark Koennecke, July 2013
 */
#include "MantidSINQ/MDHistoToWorkspace2D.h"
#include <cmath>
#include <iostream>

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MDHistoToWorkspace2D)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid;

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information, warning and error messages

void MDHistoToWorkspace2D::init() {
  declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("InputWorkspace", "",
                                                           Direction::Input));
  declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace", "",
                                                   Direction::Output));
}

void MDHistoToWorkspace2D::exec() {
  IMDHistoWorkspace_sptr inWS =
      IMDHistoWorkspace_sptr(getProperty("InputWorkspace"));

  rank = inWS->getNumDims();
  size_t nSpectra = calculateNSpectra(inWS);
  std::cout << "nSpectra = " << nSpectra << std::endl;

  boost::shared_ptr<const IMDDimension> lastDim = inWS->getDimension(rank - 1);
  std::cout << "spectraLength = " << lastDim->getNBins() << std::endl;

  Mantid::DataObjects::Workspace2D_sptr outWS;
  outWS = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(
      WorkspaceFactory::Instance().create(
          "Workspace2D", nSpectra, lastDim->getNBins(), lastDim->getNBins()));
  outWS->setYUnit("Counts");

  coord_t *pos = (coord_t *)malloc(rank * sizeof(coord_t));
  memset(pos, 0, rank * sizeof(coord_t));
  currentSpectra = 0;
  recurseData(inWS, outWS, 0, pos);
  copyMetaData(inWS, outWS);

  // checkW2D(outWS);

  setProperty("OutputWorkspace", boost::dynamic_pointer_cast<Workspace>(outWS));
}

size_t MDHistoToWorkspace2D::calculateNSpectra(IMDHistoWorkspace_sptr inWS) {
  size_t nSpectra = 1;
  for (size_t i = 0; i < rank - 1; i++) {
    boost::shared_ptr<const IMDDimension> dim = inWS->getDimension(i);
    nSpectra *= dim->getNBins();
  }
  return nSpectra;
}

void MDHistoToWorkspace2D::recurseData(IMDHistoWorkspace_sptr inWS,
                                       Workspace2D_sptr outWS,
                                       size_t currentDim, coord_t *pos) {
  boost::shared_ptr<const IMDDimension> dim = inWS->getDimension(currentDim);
  if (currentDim == rank - 1) {
    MantidVec &Y = outWS->dataY(currentSpectra);
    for (unsigned int j = 0; j < dim->getNBins(); j++) {
      pos[currentDim] = dim->getX(j);
      Y[j] = inWS->getSignalAtCoord(
          pos, static_cast<Mantid::API::MDNormalization>(0));
    }
    MantidVec &E = outWS->dataE(currentSpectra);
    // MSVC compiler can't figure out the correct overload with out the function
    // cast on sqrt
    std::transform(Y.begin(), Y.end(), E.begin(),
                   (double (*)(double))std::sqrt);
    std::vector<double> xData;
    for (unsigned int i = 0; i < dim->getNBins(); i++) {
      xData.push_back(dim->getX(i));
    }
    outWS->setX(currentSpectra, xData);
    outWS->getSpectrum(currentSpectra)
        ->setSpectrumNo(static_cast<specid_t>(currentSpectra));
    currentSpectra++;
  } else {
    // recurse deeper
    for (int i = 0; i < static_cast<int>(dim->getNBins()); i++) {
      pos[currentDim] = dim->getX(i);
      recurseData(inWS, outWS, currentDim + 1, pos);
    }
  }
}

void
MDHistoToWorkspace2D::checkW2D(Mantid::DataObjects::Workspace2D_sptr outWS) {
  size_t nSpectra = outWS->getNumberHistograms();
  size_t length = outWS->blocksize();
  MantidVec x, y, e;

  g_log.information() << "W2D has " << nSpectra << " histograms of length "
                      << length;
  for (size_t i = 0; i < nSpectra; i++) {
    ISpectrum *spec = outWS->getSpectrum(i);
    x = spec->dataX();
    y = spec->dataY();
    e = spec->dataE();
    if (x.size() != length) {
      g_log.information() << "Spectrum " << i << " x-size mismatch, is "
                          << x.size() << " should be " << length << "\n";
    }
    if (y.size() != length) {
      g_log.information() << "Spectrum " << i << " y-size mismatch, is "
                          << y.size() << " should be " << length << "\n";
    }
    if (e.size() != length) {
      g_log.information() << "Spectrum " << i << " e-size mismatch, is "
                          << e.size() << " should be " << length << "\n";
    }
  }
}

void MDHistoToWorkspace2D::copyMetaData(
    Mantid::API::IMDHistoWorkspace_sptr inWS,
    Mantid::DataObjects::Workspace2D_sptr outWS) {
  if (inWS->getNumExperimentInfo() > 0) {
    ExperimentInfo_sptr info = inWS->getExperimentInfo(0);
    outWS->copyExperimentInfoFrom(info.get());
  }
  outWS->setTitle(inWS->getTitle());
}

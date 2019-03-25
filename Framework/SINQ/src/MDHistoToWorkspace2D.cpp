// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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

#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/Logger.h"

#include <cmath>

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MDHistoToWorkspace2D)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::Points;

namespace {
Logger logger("MDHistoToWorkspace2D");
}

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information, warning and error messages

MDHistoToWorkspace2D::MDHistoToWorkspace2D()
    : Mantid::API::Algorithm(), m_rank(0), m_currentSpectra(0) {}

void MDHistoToWorkspace2D::init() {
  declareProperty(make_unique<WorkspaceProperty<IMDHistoWorkspace>>(
      "InputWorkspace", "", Direction::Input));
  declareProperty(make_unique<WorkspaceProperty<Workspace>>(
      "OutputWorkspace", "", Direction::Output));
}

void MDHistoToWorkspace2D::exec() {
  IMDHistoWorkspace_sptr inWS =
      IMDHistoWorkspace_sptr(getProperty("InputWorkspace"));

  m_rank = inWS->getNumDims();
  size_t nSpectra = calculateNSpectra(inWS);
  logger.debug() << "nSpectra = " << nSpectra << '\n';

  boost::shared_ptr<const IMDDimension> lastDim =
      inWS->getDimension(m_rank - 1);
  logger.debug() << "spectraLength = " << lastDim->getNBins() << '\n';

  Mantid::DataObjects::Workspace2D_sptr outWS;
  outWS = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(
      WorkspaceFactory::Instance().create(
          "Workspace2D", nSpectra, lastDim->getNBins(), lastDim->getNBins()));
  for (size_t i = 0; i < nSpectra; ++i)
    outWS->getSpectrum(i).setDetectorID(static_cast<detid_t>(i + 1));
  outWS->setYUnit("Counts");

  coord_t *pos = reinterpret_cast<coord_t *>(malloc(m_rank * sizeof(coord_t)));
  memset(pos, 0, m_rank * sizeof(coord_t));
  m_currentSpectra = 0;
  recurseData(inWS, outWS, 0, pos);
  copyMetaData(inWS, outWS);

  // checkW2D(outWS);

  setProperty("OutputWorkspace", boost::dynamic_pointer_cast<Workspace>(outWS));
}

size_t MDHistoToWorkspace2D::calculateNSpectra(IMDHistoWorkspace_sptr inWS) {
  size_t nSpectra = 1;
  for (size_t i = 0; i < m_rank - 1; i++) {
    boost::shared_ptr<const IMDDimension> dim = inWS->getDimension(i);
    nSpectra *= dim->getNBins();
  }
  return nSpectra;
}

void MDHistoToWorkspace2D::recurseData(IMDHistoWorkspace_sptr inWS,
                                       Workspace2D_sptr outWS,
                                       size_t currentDim, coord_t *pos) {
  boost::shared_ptr<const IMDDimension> dim = inWS->getDimension(currentDim);
  if (currentDim == m_rank - 1) {
    Counts counts(dim->getNBins());
    auto &Y = counts.mutableData();

    for (unsigned int j = 0; j < dim->getNBins(); j++) {
      pos[currentDim] = dim->getX(j);
      Y[j] = inWS->getSignalAtCoord(
          pos, static_cast<Mantid::API::MDNormalization>(0));
    }

    Points points(dim->getNBins());
    auto &xData = points.mutableData();
    for (unsigned int i = 0; i < dim->getNBins(); i++) {
      xData[i] = dim->getX(i);
    }

    outWS->setHistogram(m_currentSpectra, std::move(points), std::move(counts));
    outWS->getSpectrum(m_currentSpectra)
        .setSpectrumNo(static_cast<specnum_t>(m_currentSpectra));
    m_currentSpectra++;
  } else {
    // recurse deeper
    for (int i = 0; i < static_cast<int>(dim->getNBins()); i++) {
      pos[currentDim] = dim->getX(i);
      recurseData(inWS, outWS, currentDim + 1, pos);
    }
  }
}

void MDHistoToWorkspace2D::checkW2D(
    Mantid::DataObjects::Workspace2D_sptr outWS) {
  size_t nSpectra = outWS->getNumberHistograms();
  size_t length = outWS->blocksize();

  g_log.information() << "W2D has " << nSpectra << " histograms of length "
                      << length;
  for (size_t i = 0; i < nSpectra; i++) {
    auto &spec = outWS->getSpectrum(i);
    auto &x = spec.x();
    auto &y = spec.y();
    auto &e = spec.e();
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

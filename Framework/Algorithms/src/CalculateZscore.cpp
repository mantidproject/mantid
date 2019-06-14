// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CalculateZscore.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/HistogramBuilder.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Statistics.h"

#include <sstream>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;
using namespace std;

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(CalculateZscore)

//----------------------------------------------------------------------------------------------
/** Define properties
 */
void CalculateZscore::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "Anonymous", Direction::Input),
                  "Name of input MatrixWorkspace to have Z-score calculated.");

  declareProperty(std::make_unique<WorkspaceProperty<Workspace2D>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name of the output Workspace2D containing the Z-scores.");

  declareProperty("WorkspaceIndex", EMPTY_INT(),
                  "Index of the spetrum to have Z-score calculated. "
                  "Default is to calculate for all spectra.");
}

//----------------------------------------------------------------------------------------------
/** Execute body
 */
void CalculateZscore::exec() {
  // 1. Get input and validate
  MatrixWorkspace_const_sptr inpWS = getProperty("InputWorkspace");
  int inpwsindex = getProperty("WorkspaceIndex");

  bool zscoreforall = false;
  if (inpwsindex == EMPTY_INT()) {
    zscoreforall = true;
  }

  // 2. Generate output
  size_t numspec;
  if (zscoreforall) {
    numspec = inpWS->getNumberHistograms();
  } else {
    numspec = 1;
  }
  size_t sizex = inpWS->x(0).size();
  size_t sizey = inpWS->y(0).size();

  HistogramBuilder builder;
  builder.setX(sizex);
  builder.setY(sizey);
  builder.setDistribution(inpWS->isDistribution());
  Workspace2D_sptr outWS = create<Workspace2D>(numspec, builder.build());

  Progress progress(this, 0.0, 1.0, numspec);

  // 3. Get Z values
  for (size_t i = 0; i < numspec; ++i) {
    // a) figure out wsindex
    size_t wsindex;
    if (zscoreforall) {
      // Update wsindex to index in input workspace
      wsindex = i;
    } else {
      // Use the wsindex as the input
      wsindex = static_cast<size_t>(inpwsindex);
      if (wsindex >= inpWS->getNumberHistograms()) {
        stringstream errmsg;
        errmsg << "Input workspace index " << inpwsindex
               << " is out of input workspace range = "
               << inpWS->getNumberHistograms() << '\n';
      }
    }

    // b) Calculate Zscore
    auto &inpY = inpWS->y(wsindex).rawData();
    auto &inpE = inpWS->e(wsindex).rawData();

    auto &histY = outWS->mutableY(i);
    auto &histE = outWS->mutableE(i);

    vector<double> yzscores = getZscore(inpY);
    vector<double> ezscores = getZscore(inpE);

    outWS->setSharedX(i, inpWS->sharedX(wsindex));
    histY = yzscores;
    histE = ezscores;

    progress.report("Calculating Z Score");
  } // ENDFOR

  // 4. Set the output
  setProperty("OutputWorkspace", outWS);
}

} // namespace Algorithms
} // namespace Mantid

#include "MantidSINQ/SINQTranspose3D.h"
#include "MantidKernel/ListValidator.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidMDEvents/MDHistoWorkspace.h"

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SINQTranspose3D)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::MDEvents;

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information, warning and error messages

void SINQTranspose3D::init() {
  declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("InputWorkspace", "",
                                                           Direction::Input));
  std::vector<std::string> transposeOptions;
  transposeOptions.push_back("Y,X,Z");
  transposeOptions.push_back("X,Z,Y");
  transposeOptions.push_back("TRICS");
  transposeOptions.push_back("AMOR");
  this->declareProperty(
      "TransposeOption", "Y,X,Z",
      boost::make_shared<StringListValidator>(transposeOptions),
      "The transpose option");

  declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace", "",
                                                   Direction::Output));
}

void SINQTranspose3D::exec() {
  IMDHistoWorkspace_sptr inWS =
      IMDHistoWorkspace_sptr(getProperty("InputWorkspace"));

  std::string transposeOption = getProperty("TransposeOption");

  if (inWS->getNumDims() != 3) {
    throw std::runtime_error(
        "This algorithm only works with MDHistoWorkspaces of rank 3!");
  }

  if (transposeOption == "Y,X,Z") {
    doYXZ(inWS);
  } else if (transposeOption == "X,Z,Y") {
    doXZY(inWS);
  } else if (transposeOption == "TRICS") {
    doTRICS(inWS);
  } else if (transposeOption == "AMOR") {
    doAMOR(inWS);
  } else {
    throw std::runtime_error("Transpose Option not found!");
  }
}

void SINQTranspose3D::doYXZ(IMDHistoWorkspace_sptr inWS) {
  double *inVal, *inErr, *outVal, *outErr;
  size_t idxIn, idxOut;

  boost::shared_ptr<const IMDDimension> x, y, z;
  x = inWS->getXDimension();
  y = inWS->getYDimension();
  z = inWS->getZDimension();

  std::vector<IMDDimension_sptr> dimensions;
  dimensions.push_back(boost::const_pointer_cast<IMDDimension>(y));
  dimensions.push_back(boost::const_pointer_cast<IMDDimension>(x));
  dimensions.push_back(boost::const_pointer_cast<IMDDimension>(z));

  MDHistoWorkspace_sptr outWS(new MDHistoWorkspace(dimensions));

  inVal = inWS->getSignalArray();
  inErr = inWS->getErrorSquaredArray();
  outVal = outWS->getSignalArray();
  outErr = outWS->getErrorSquaredArray();
  for (unsigned int xx = 0; xx < x->getNBins(); xx++) {
    for (unsigned int yy = 0; yy < y->getNBins(); yy++) {
      for (unsigned int zz = 0; zz < z->getNBins(); zz++) {
        idxIn = inWS->getLinearIndex(xx, yy, zz);
        idxOut = outWS->getLinearIndex(yy, xx, zz);
        outVal[idxOut] = inVal[idxIn];
        outErr[idxOut] = inErr[idxIn];
      }
    }
  }
  copyMetaData(inWS, outWS);

  // assign the workspace
  setProperty("OutputWorkspace", outWS);
}

void SINQTranspose3D::doXZY(IMDHistoWorkspace_sptr inWS) {
  double *inVal, *inErr, *outVal, *outErr;
  size_t idxIn, idxOut;
  unsigned int xdim, ydim, zdim;

  boost::shared_ptr<const IMDDimension> x, y, z;
  x = inWS->getXDimension();
  y = inWS->getYDimension();
  z = inWS->getZDimension();

  std::vector<IMDDimension_sptr> dimensions;
  dimensions.push_back(boost::const_pointer_cast<IMDDimension>(x));
  dimensions.push_back(boost::const_pointer_cast<IMDDimension>(z));
  dimensions.push_back(boost::const_pointer_cast<IMDDimension>(y));

  MDHistoWorkspace_sptr outWS(new MDHistoWorkspace(dimensions));

  inVal = inWS->getSignalArray();
  inErr = inWS->getErrorSquaredArray();
  outVal = outWS->getSignalArray();
  outErr = outWS->getErrorSquaredArray();
  xdim = static_cast<unsigned int>(x->getNBins());
  ydim = static_cast<unsigned int>(y->getNBins());
  zdim = static_cast<unsigned int>(z->getNBins());
  for (unsigned int xx = 0; xx < xdim; xx++) {
    for (unsigned int yy = 0; yy < ydim; yy++) {
      for (unsigned int zz = 0; zz < zdim; zz++) {
        idxIn = inWS->getLinearIndex(xx, yy, zz);
        idxOut = outWS->getLinearIndex(xx, zz, yy);
        outVal[idxOut] = inVal[idxIn];
        outErr[idxOut] = inErr[idxIn];
      }
    }
  }
  copyMetaData(inWS, outWS);

  // assign the workspace
  setProperty("OutputWorkspace", outWS);
}
void SINQTranspose3D::doTRICS(IMDHistoWorkspace_sptr inWS) {
  double *inVal, *inErr, *outVal, *outErr;
  size_t idxIn, idxOut;
  unsigned int xdim, ydim, zdim;

  boost::shared_ptr<const IMDDimension> x, y, z;
  x = inWS->getXDimension();
  y = inWS->getYDimension();
  z = inWS->getZDimension();

  std::vector<IMDDimension_sptr> dimensions;
  dimensions.push_back(boost::const_pointer_cast<IMDDimension>(x));
  dimensions.push_back(boost::const_pointer_cast<IMDDimension>(z));
  dimensions.push_back(boost::const_pointer_cast<IMDDimension>(y));

  MDHistoWorkspace_sptr outWS(new MDHistoWorkspace(dimensions));
  outWS->setTo(.0, .0, .0);

  inVal = inWS->getSignalArray();
  inErr = inWS->getErrorSquaredArray();
  outVal = outWS->getSignalArray();
  outErr = outWS->getErrorSquaredArray();
  xdim = static_cast<unsigned int>(x->getNBins());
  ydim = static_cast<unsigned int>(y->getNBins());
  zdim = static_cast<unsigned int>(z->getNBins());
  for (unsigned int xx = 0; xx < xdim; xx++) {
    for (unsigned int yy = 0; yy < ydim; yy++) {
      for (unsigned int zz = 0; zz < zdim; zz++) {
        idxIn = ydim * zdim * xx + zdim * yy + zz; // this works for TRICS
        idxOut = outWS->getLinearIndex(xx, zz, yy);
        outVal[idxOut] = inVal[idxIn];
        outErr[idxOut] = inErr[idxIn];
      }
    }
  }
  copyMetaData(inWS, outWS);

  // assign the workspace
  setProperty("OutputWorkspace", outWS);
}
void SINQTranspose3D::doAMOR(IMDHistoWorkspace_sptr inWS) {
  double val, *inVal;
  unsigned int xdim, ydim, zdim, idx;

  boost::shared_ptr<const IMDDimension> x, y, z;
  x = inWS->getXDimension();
  y = inWS->getYDimension();
  z = inWS->getZDimension();

  std::vector<IMDDimension_sptr> dimensions;
  dimensions.push_back(boost::const_pointer_cast<IMDDimension>(y));
  dimensions.push_back(boost::const_pointer_cast<IMDDimension>(x));
  dimensions.push_back(boost::const_pointer_cast<IMDDimension>(z));

  MDHistoWorkspace_sptr outWS(new MDHistoWorkspace(dimensions));
  outWS->setTo(.0, .0, .0);

  xdim = static_cast<unsigned int>(x->getNBins());
  ydim = static_cast<unsigned int>(y->getNBins());
  zdim = static_cast<unsigned int>(z->getNBins());
  inVal = inWS->getSignalArray();
  for (unsigned int xx = 0; xx < xdim; xx++) {
    for (unsigned int yy = 0; yy < ydim; yy++) {
      for (unsigned zz = 0; zz < zdim; zz++) {
        // idx = ydim*zdim*xx + zdim*yy + zz;
        idx = ydim * zdim * xx + zdim * yy + zz;
        val = inVal[idx];
        outWS->setSignalAt(outWS->getLinearIndex(yy, xx, zz), val);
        outWS->setErrorSquaredAt(outWS->getLinearIndex(yy, xx, zz), val);
      }
    }
  }

  copyMetaData(inWS, outWS);

  // assign the workspace
  setProperty("OutputWorkspace", outWS);
}

void SINQTranspose3D::copyMetaData(Mantid::API::IMDHistoWorkspace_sptr inws,
                                   Mantid::API::IMDHistoWorkspace_sptr outws) {
  outws->setTitle(inws->getTitle());
  ExperimentInfo_sptr info;

  if (inws->getNumExperimentInfo() > 0) {
    info = inws->getExperimentInfo(0);
    outws->addExperimentInfo(info);
  }
}

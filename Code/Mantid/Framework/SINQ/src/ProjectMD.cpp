#include "MantidSINQ/ProjectMD.h"
#include "MantidKernel/ListValidator.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidMDEvents/MDHistoWorkspace.h"

#define MAXDIM 10

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ProjectMD)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::MDEvents;

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information, warning and error messages

void ProjectMD::init() {
  declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("InputWorkspace", "",
                                                           Direction::Input));
  std::vector<std::string> projectOptions;
  projectOptions.push_back("X");
  projectOptions.push_back("Y");
  projectOptions.push_back("Z");
  projectOptions.push_back("K");
  this->declareProperty("ProjectDirection", "Z",
                        boost::make_shared<StringListValidator>(projectOptions),
                        "The project direction");

  declareProperty("StartIndex", 0, Direction::Input);
  declareProperty("EndIndex", -1, Direction::Input);

  declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace", "",
                                                   Direction::Output));
}

void ProjectMD::exec() {
  IMDHistoWorkspace_sptr inWS =
      IMDHistoWorkspace_sptr(getProperty("InputWorkspace"));
  unsigned int dimNo;
  int start, end, targetDim[MAXDIM], sourceDim[MAXDIM];

  std::string projectDirection = getProperty("ProjectDirection");
  if (projectDirection == "X") {
    dimNo = 0;
  } else if (projectDirection == "Y") {
    dimNo = 1;
  } else if (projectDirection == "Z") {
    dimNo = 2;
  } else if (projectDirection == "K") {
    dimNo = 3;
  } else {
    throw std::runtime_error("ProjectDirection not recognized");
  }
  start = getProperty("StartIndex");
  end = getProperty("EndIndex");

  std::vector<IMDDimension_sptr> dimensions;
  for (size_t i = 0; i < inWS->getNumDims(); i++) {
    if (i != dimNo) {
      dimensions.push_back(
          boost::const_pointer_cast<IMDDimension>(inWS->getDimension(i)));
    } else {
      boost::shared_ptr<const IMDDimension> dimi = inWS->getDimension(i);
      if (start < 0) {
        start = 0;
      }
      if (end == -1) {
        end = static_cast<int>(dimi->getNBins());
      } else if (end > int(dimi->getNBins())) {
        end = static_cast<int>(dimi->getNBins()) - 1;
      }
    }
  }

  MDHistoWorkspace_sptr outWS(new MDHistoWorkspace(dimensions));
  outWS->setTo(.0, .0, .0);

  memset(targetDim, 0, MAXDIM * sizeof(int));
  memset(sourceDim, 0, MAXDIM * sizeof(int));
  sumData(inWS, outWS, sourceDim, targetDim, 0, static_cast<int>(dimNo), start,
          end, 0);

  copyMetaData(inWS, outWS);

  // assign the workspace
  setProperty("OutputWorkspace", outWS);
}
void ProjectMD::copyMetaData(Mantid::API::IMDHistoWorkspace_sptr inws,
                             Mantid::API::IMDHistoWorkspace_sptr outws) {
  outws->setTitle(inws->getTitle());
  ExperimentInfo_sptr info;

  if (inws->getNumExperimentInfo() > 0) {
    info = inws->getExperimentInfo(0);
    outws->addExperimentInfo(info);
  }
}
/**
 * This looks stupid but:
 *    - ws->getIndexMultiplier() is not in the interface but only in the
 * implementation
 *      of MDHistoWorkspace. Use would require some shitty casting
 *    - From the docs it is not clear if MDHistoWorkspace is in C or F77 storage
 * order.
 *      The documentation actually suggest F77, the code too?
 *  This isolates this from the storage order concern.
 */
unsigned int ProjectMD::calcIndex(IMDHistoWorkspace_sptr ws, int dim[]) {
  size_t idx = 0;
  switch (ws->getNumDims()) {
  case 1:
    idx = dim[0];
    break;
  case 2:
    idx = ws->getLinearIndex(dim[0], dim[1]);
    break;
  case 3:
    idx = ws->getLinearIndex(dim[0], dim[1], dim[2]);
    break;
  case 4:
    idx = ws->getLinearIndex(dim[0], dim[1], dim[2], dim[3]);
    break;
  default:
    throw std::runtime_error("Unsupported dimension depth");
  }
  return static_cast<unsigned int>(idx);
}

double ProjectMD::getValue(IMDHistoWorkspace_sptr ws, int dim[]) {
  unsigned int idx = calcIndex(ws, dim);
  double val = ws->signalAt(idx);
  // double *a = ws->getSignalArray();
  // double val = a[idx];
  // std::cout << "index " << idx << " value " << val << " dims " << dim[0] <<",
  // " << dim[1] << "," <<dim[2] <<std::endl;
  return val;
}
void ProjectMD::putValue(IMDHistoWorkspace_sptr ws, int dim[], double value) {
  unsigned int idx = calcIndex(ws, dim);
  // std::cout << "Result index " << idx << " value " << value << " dim= " <<
  // dim[0] << ", " << dim[1] <<", " << dim[2] <<std::endl;
  ws->setSignalAt(idx, value);
  ws->setErrorSquaredAt(idx, value);
}

void ProjectMD::sumData(IMDHistoWorkspace_sptr inWS,
                        IMDHistoWorkspace_sptr outWS, int *sourceDim,
                        int *targetDim, int targetDimCount, int dimNo,
                        int start, int end, int currentDim) {
  boost::shared_ptr<const IMDDimension> dimi;

  /*
     when we have recursed through  all dimensions
     we actually do the sums...
   */
  if (currentDim == static_cast<int>(inWS->getNumDims())) {
    int length = end - start;
    double sumVal = getValue(outWS, targetDim);
    for (int i = 0; i < length; i++) {
      sourceDim[dimNo] = start + i;
      double val = getValue(inWS, sourceDim);
      sumVal += val;
    }
    putValue(outWS, targetDim, sumVal);
  } else {
    /*
       jump over the summed dimension while recursing
       through the dimensions
     */
    if (currentDim == dimNo) {
      sumData(inWS, outWS, sourceDim, targetDim, targetDimCount, dimNo, start,
              end, currentDim + 1);
    } else {
      /*
         loop over all values of the non summed dimension
       */
      dimi = inWS->getDimension(currentDim);
      for (int i = 0; i < static_cast<int>(dimi->getNBins()); i++) {
        /*
           the problem here is that we have to jump over the summed
           dimension here. This why we have to maintain a separate
           dimension count for the target array. Jumping is done
           above.
         */
        targetDim[targetDimCount] = i;
        targetDimCount++;

        sourceDim[currentDim] = i;
        sumData(inWS, outWS, sourceDim, targetDim, targetDimCount, dimNo, start,
                end, currentDim + 1);
        targetDimCount--;
      }
    }
  }
}

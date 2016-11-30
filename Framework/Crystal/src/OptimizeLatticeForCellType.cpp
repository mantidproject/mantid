#include "MantidCrystal/OptimizeLatticeForCellType.h"
#include "MantidCrystal/GSLFunctions.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/Sample.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Crystal/ReducedCell.h"
#include <fstream>

using namespace Mantid::Geometry;

namespace Mantid {
namespace Crystal {
// Register the class into the algorithm factory
DECLARE_ALGORITHM(OptimizeLatticeForCellType)

using namespace Kernel;
using namespace API;
using std::size_t;
using namespace DataObjects;

/** Initialisation method. Declares properties to be used in algorithm.
 */
void OptimizeLatticeForCellType::init() {

  declareProperty(make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "PeaksWorkspace", "", Direction::InOut),
                  "An input PeaksWorkspace with an instrument.");
  std::vector<std::string> cellTypes;
  cellTypes.push_back(ReducedCell::CUBIC());
  cellTypes.push_back(ReducedCell::TETRAGONAL());
  cellTypes.push_back(ReducedCell::ORTHORHOMBIC());
  cellTypes.push_back(ReducedCell::HEXAGONAL());
  cellTypes.push_back(ReducedCell::RHOMBOHEDRAL());
  cellTypes.push_back(ReducedCell::MONOCLINIC());
  cellTypes.push_back(ReducedCell::TRICLINIC());
  declareProperty("CellType", cellTypes[0],
                  boost::make_shared<StringListValidator>(cellTypes),
                  "Select the cell type.");
  declareProperty("Apply", false, "Re-index the peaks");
  declareProperty("PerRun", false, "Make per run orientation matrices");
  declareProperty("Tolerance", 0.12, "Indexing Tolerance");
  declareProperty("EdgePixels", 0,
                  "Remove peaks that are at pixels this close to edge. ");
  declareProperty(make_unique<PropertyWithValue<double>>("OutputChi2", 0.0,
                                                         Direction::Output),
                  "Returns the goodness of the fit");
  declareProperty(
      make_unique<FileProperty>("OutputDirectory", ".",
                                FileProperty::Directory),
      "The directory where the per run peaks files and orientation matrices "
      "will be written.");

  // Disable default gsl error handler (which is to call abort!)
  gsl_set_error_handler_off();
}

/** Executes the algorithm
 *
 *  @throw Exception::FileError If the grouping file cannot be opened or read
 *successfully
 */
void OptimizeLatticeForCellType::exec() {
  bool apply = this->getProperty("Apply");
  bool perRun = this->getProperty("PerRun");
  double tolerance = this->getProperty("Tolerance");
  int edge = this->getProperty("EdgePixels");
  std::string cell_type = getProperty("CellType");
  DataObjects::PeaksWorkspace_sptr ws = getProperty("PeaksWorkspace");

  std::vector<DataObjects::PeaksWorkspace_sptr> runWS;
  if (edge > 0) {
    for (int i = int(ws->getNumberPeaks()) - 1; i >= 0; --i) {
      const std::vector<Peak> &peaks = ws->getPeaks();
      if (edgePixel(ws, peaks[i].getBankName(), peaks[i].getCol(),
                    peaks[i].getRow(), edge)) {
        ws->removePeak(i);
      }
    }
  }
  runWS.push_back(ws);

  if (perRun) {
    std::vector<std::pair<std::string, bool>> criteria;
    // Sort by run number
    criteria.push_back(std::pair<std::string, bool>("runnumber", true));
    ws->sort(criteria);
    const std::vector<Peak> &peaks_all = ws->getPeaks();
    int run = 0;
    int count = 0;
    for (const auto &peak : peaks_all) {
      if (peak.getRunNumber() != run) {
        count++; // first entry in runWS is input workspace
        auto cloneWS = boost::make_shared<PeaksWorkspace>();
        cloneWS->setInstrument(ws->getInstrument());
        cloneWS->copyExperimentInfoFrom(ws.get());
        runWS.push_back(cloneWS);
        runWS[count]->addPeak(peak);
        run = peak.getRunNumber();
        AnalysisDataService::Instance().addOrReplace(
            std::to_string(run) + ws->getName(), runWS[count]);
      } else {
        runWS[count]->addPeak(peak);
      }
    }
  }
  // finally do the optimization
  for (auto &i_run : runWS) {
    DataObjects::PeaksWorkspace_sptr peakWS(i_run->clone());
    AnalysisDataService::Instance().addOrReplace("_peaks", peakWS);
    const DblMatrix UB = peakWS->sample().getOrientedLattice().getUB();
    std::vector<double> lat(6);
    IndexingUtils::GetLatticeParameters(UB, lat);

    API::ILatticeFunction_sptr latticeFunction =
        getLatticeFunction(cell_type, peakWS->sample().getOrientedLattice());

    IAlgorithm_sptr fit_alg;
    try {
      fit_alg = createChildAlgorithm("Fit", -1, -1, false);
    } catch (Exception::NotFoundError &) {
      g_log.error("Can't locate Fit algorithm");
      throw;
    }

    fit_alg->setProperty(
        "Function", boost::static_pointer_cast<IFunction>(latticeFunction));
    fit_alg->setProperty("Ties", "ZeroShift=0.0");
    fit_alg->setProperty("InputWorkspace", peakWS);
    fit_alg->setProperty("CostFunction", "Unweighted least squares");
    fit_alg->setProperty("CreateOutput", true);
    fit_alg->executeAsChildAlg();

    double chisq = fit_alg->getProperty("OutputChi2overDoF");
    Geometry::UnitCell refinedCell = latticeFunction->getUnitCell();

    IAlgorithm_sptr ub_alg;
    try {
      ub_alg = createChildAlgorithm("CalculateUMatrix", -1, -1, false);
    } catch (Exception::NotFoundError &) {
      g_log.error("Can't locate CalculateUMatrix algorithm");
      throw;
    }

    ub_alg->setProperty("PeaksWorkspace", peakWS);
    ub_alg->setProperty("a", refinedCell.a());
    ub_alg->setProperty("b", refinedCell.b());
    ub_alg->setProperty("c", refinedCell.c());
    ub_alg->setProperty("alpha", refinedCell.alpha());
    ub_alg->setProperty("beta", refinedCell.beta());
    ub_alg->setProperty("gamma", refinedCell.gamma());
    ub_alg->executeAsChildAlg();
    DblMatrix UBnew = peakWS->mutableSample().getOrientedLattice().getUB();
    OrientedLattice o_lattice;
    o_lattice.setUB(UBnew);
    o_lattice.set(refinedCell.a(), refinedCell.b(), refinedCell.c(),
                  refinedCell.alpha(), refinedCell.beta(), refinedCell.gamma());
    o_lattice.setError(refinedCell.errora(), refinedCell.errorb(),
                       refinedCell.errorc(), refinedCell.erroralpha(),
                       refinedCell.errorbeta(), refinedCell.errorgamma());

    // Show the modified lattice parameters
    g_log.notice() << i_run->getName() << "  " << o_lattice << "\n";

    i_run->mutableSample().setOrientedLattice(&o_lattice);

    setProperty("OutputChi2", chisq);

    if (apply) {
      // Reindex peaks with new UB
      Mantid::API::IAlgorithm_sptr alg = createChildAlgorithm("IndexPeaks");
      alg->setPropertyValue("PeaksWorkspace", i_run->getName());
      alg->setProperty("Tolerance", tolerance);
      alg->executeAsChildAlg();
    }
    AnalysisDataService::Instance().remove("_peaks");
    if (perRun) {
      std::string outputdir = getProperty("OutputDirectory");
      if (outputdir.back() != '/')
        outputdir += "/";
      // Save Peaks
      Mantid::API::IAlgorithm_sptr savePks_alg =
          createChildAlgorithm("SaveIsawPeaks");
      savePks_alg->setPropertyValue("InputWorkspace", i_run->getName());
      savePks_alg->setProperty("Filename", outputdir + "ls" + i_run->getName() +
                                               ".integrate");
      savePks_alg->executeAsChildAlg();
      g_log.notice() << "See output file: "
                     << outputdir + "ls" + i_run->getName() + ".integrate"
                     << "\n";
      // Save UB
      Mantid::API::IAlgorithm_sptr saveUB_alg =
          createChildAlgorithm("SaveIsawUB");
      saveUB_alg->setPropertyValue("InputWorkspace", i_run->getName());
      saveUB_alg->setProperty("Filename",
                              outputdir + "ls" + i_run->getName() + ".mat");
      saveUB_alg->executeAsChildAlg();
      // Show the names of files written
      g_log.notice() << "See output file: "
                     << outputdir + "ls" + i_run->getName() + ".mat"
                     << "\n";
    }
  }
}
//-----------------------------------------------------------------------------------------
/**
  @param  cellType                cell type to optimize
  @param  cell                          unit cell
  @return  latticeFunction        Function for fitting
*/
API::ILatticeFunction_sptr
OptimizeLatticeForCellType::getLatticeFunction(const std::string &cellType,
                                               const UnitCell &cell) const {
  std::ostringstream fun_str;
  fun_str << "name=LatticeFunction,LatticeSystem=" << cellType;

  API::IFunction_sptr rawFunction =
      API::FunctionFactory::Instance().createInitialized(fun_str.str());
  API::ILatticeFunction_sptr latticeFunction =
      boost::dynamic_pointer_cast<API::ILatticeFunction>(rawFunction);
  if (latticeFunction) {
    latticeFunction->setUnitCell(cell);
  }

  return latticeFunction;
}

//-----------------------------------------------------------------------------------------
/**
  @param  ws           Name of workspace containing peaks
  @param  bankName     Name of bank containing peak
  @param  col          Column number containing peak
  @param  row          Row number containing peak
  @param  Edge         Number of edge points for each bank
  @return True if peak is on edge
*/
bool OptimizeLatticeForCellType::edgePixel(PeaksWorkspace_sptr ws,
                                           std::string bankName, int col,
                                           int row, int Edge) {
  if (bankName.compare("None") == 0)
    return false;
  Geometry::Instrument_const_sptr Iptr = ws->getInstrument();
  boost::shared_ptr<const IComponent> parent =
      Iptr->getComponentByName(bankName);
  if (parent->type().compare("RectangularDetector") == 0) {
    boost::shared_ptr<const RectangularDetector> RDet =
        boost::dynamic_pointer_cast<const RectangularDetector>(parent);

    return col < Edge || col >= (RDet->xpixels() - Edge) || row < Edge ||
           row >= (RDet->ypixels() - Edge);
  } else {
    std::vector<Geometry::IComponent_const_sptr> children;
    boost::shared_ptr<const Geometry::ICompAssembly> asmb =
        boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(parent);
    asmb->getChildren(children, false);
    int startI = 1;
    if (children[0]->getName() == "sixteenpack") {
      startI = 0;
      parent = children[0];
      children.clear();
      boost::shared_ptr<const Geometry::ICompAssembly> asmb =
          boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(parent);
      asmb->getChildren(children, false);
    }
    boost::shared_ptr<const Geometry::ICompAssembly> asmb2 =
        boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(children[0]);
    std::vector<Geometry::IComponent_const_sptr> grandchildren;
    asmb2->getChildren(grandchildren, false);
    int NROWS = static_cast<int>(grandchildren.size());
    int NCOLS = static_cast<int>(children.size());
    // Wish pixels and tubes start at 1 not 0
    return col - startI < Edge || col - startI >= (NCOLS - Edge) ||
           row - startI < Edge || row - startI >= (NROWS - Edge);
  }
  return false;
}

} // namespace Algorithm
} // namespace Mantid

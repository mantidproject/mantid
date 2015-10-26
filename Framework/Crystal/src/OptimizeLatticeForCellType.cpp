#include "MantidCrystal/OptimizeLatticeForCellType.h"
#include "MantidCrystal/GSLFunctions.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
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

/// Constructor
OptimizeLatticeForCellType::OptimizeLatticeForCellType() {}

/// Destructor
OptimizeLatticeForCellType::~OptimizeLatticeForCellType() {}

//-----------------------------------------------------------------------------------------
/** Initialisation method. Declares properties to be used in algorithm.
 */
void OptimizeLatticeForCellType::init() {

  declareProperty(new WorkspaceProperty<PeaksWorkspace>("PeaksWorkspace", "",
                                                        Direction::InOut),
                  "An input PeaksWorkspace with an instrument.");
  std::vector<std::string> cellTypes;
  cellTypes.push_back(ReducedCell::CUBIC());
  cellTypes.push_back(ReducedCell::TETRAGONAL());
  cellTypes.push_back(ReducedCell::ORTHORHOMBIC());
  cellTypes.push_back(ReducedCell::HEXAGONAL());
  cellTypes.push_back("Trigonal"); // was RHOMBOHEDRAL
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
  declareProperty(
      new PropertyWithValue<double>("OutputChi2", 0.0, Direction::Output),
      "Returns the goodness of the fit");
  declareProperty(
      new FileProperty("OutputDirectory", ".", FileProperty::Directory),
      "The directory where the per run peaks files and orientation matrices "
      "will be written.");

  // Disable default gsl error handler (which is to call abort!)
  gsl_set_error_handler_off();
}

//-----------------------------------------------------------------------------------------
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

  for (int i = int(ws->getNumberPeaks()) - 1; i >= 0; --i) {
    const std::vector<Peak> &peaks = ws->getPeaks();
    if (edgePixel(ws, peaks[i].getBankName(), peaks[i].getCol(),
                  peaks[i].getRow(), edge)) {
      ws->removePeak(i);
    }
  }
  runWS.push_back(ws);

  if (perRun) {
    std::vector<std::pair<std::string, bool> > criteria;
    // Sort by run number
    criteria.push_back(std::pair<std::string, bool>("runnumber", true));
    ws->sort(criteria);
    const std::vector<Peak> &peaks_all = ws->getPeaks();
    int run = 0;
    int count = 0;
    for (size_t i = 0; i < peaks_all.size(); i++) {
      if (peaks_all[i].getRunNumber() != run) {
        count++; // first entry in runWS is input workspace
        DataObjects::PeaksWorkspace_sptr cloneWS(new PeaksWorkspace());
        cloneWS->setInstrument(ws->getInstrument());
        cloneWS->copyExperimentInfoFrom(ws.get());
        runWS.push_back(cloneWS);
        runWS[count]->addPeak(peaks_all[i]);
        run = peaks_all[i].getRunNumber();
        AnalysisDataService::Instance().addOrReplace(
            boost::lexical_cast<std::string>(run) + ws->getName(),
            runWS[count]);
      } else {
        runWS[count]->addPeak(peaks_all[i]);
      }
    }
  }
  // finally do the optimization
  for (size_t i_run = 0; i_run < runWS.size(); i_run++) {
    DataObjects::PeaksWorkspace_sptr peakWS(runWS[i_run]->clone().release());
    AnalysisDataService::Instance().addOrReplace("_peaks", peakWS);
    const DblMatrix UB = peakWS->sample().getOrientedLattice().getUB();
    std::vector<double> lat(6);
    IndexingUtils::GetLatticeParameters(UB, lat);

    std::string fun_str = inParams(cell_type, lat);

    IAlgorithm_sptr fit_alg;
    try {
      fit_alg = createChildAlgorithm("Fit", -1, -1, false);
    }
    catch (Exception::NotFoundError &) {
      g_log.error("Can't locate Fit algorithm");
      throw;
    }

    fit_alg->setPropertyValue("Function", fun_str);
    fit_alg->setProperty("Ties", "ZeroShift=0.0");
    fit_alg->setProperty("InputWorkspace", peakWS);
    fit_alg->setProperty("CostFunction", "Unweighted least squares");
    fit_alg->setProperty("CreateOutput", true);
    fit_alg->setProperty("Output", "fit");
    fit_alg->executeAsChildAlg();

    double chisq = fit_alg->getProperty("OutputChi2overDoF");
    ITableWorkspace_sptr ParamTable = fit_alg->getProperty("OutputParameters");
    std::vector<double> Params = outParams(cell_type, 1, ParamTable);

    std::vector<double> sigabc = outParams(cell_type, 2, ParamTable);

    IAlgorithm_sptr ub_alg;
    try {
      ub_alg =
          createChildAlgorithm("FindUBUsingLatticeParameters", -1, -1, false);
    }
    catch (Exception::NotFoundError &) {
      g_log.error("Can't locate FindUBUsingLatticeParameters algorithm");
      throw;
    }

    ub_alg->setProperty("PeaksWorkspace", peakWS);
    ub_alg->setProperty("a", Params[0]);
    ub_alg->setProperty("b", Params[1]);
    ub_alg->setProperty("c", Params[2]);
    ub_alg->setProperty("alpha", Params[3]);
    ub_alg->setProperty("beta", Params[4]);
    ub_alg->setProperty("gamma", Params[5]);
    ub_alg->setProperty("NumInitial", 15);
    ub_alg->setProperty("Tolerance", tolerance);
    ub_alg->executeAsChildAlg();
    DblMatrix UBnew = peakWS->mutableSample().getOrientedLattice().getUB();
    OrientedLattice o_lattice;
    o_lattice.setUB(UBnew);
    o_lattice.set(Params[0], Params[1], Params[2], Params[3], Params[4],
                  Params[5]);
    o_lattice.setError(sigabc[0], sigabc[1], sigabc[2], sigabc[3], sigabc[4],
                       sigabc[5]);

    // Show the modified lattice parameters
    g_log.notice() << runWS[i_run]->getName() << "  " << o_lattice << "\n";

    runWS[i_run]->mutableSample().setOrientedLattice(&o_lattice);

    setProperty("OutputChi2", chisq);

    if (apply) {
      // Reindex peaks with new UB
      Mantid::API::IAlgorithm_sptr alg = createChildAlgorithm("IndexPeaks");
      alg->setPropertyValue("PeaksWorkspace", runWS[i_run]->getName());
      alg->setProperty("Tolerance", tolerance);
      alg->executeAsChildAlg();
    }
    AnalysisDataService::Instance().remove("_peaks");
    if (perRun) {
      std::string outputdir = getProperty("OutputDirectory");
      if (outputdir[outputdir.size() - 1] != '/')
        outputdir += "/";
      // Save Peaks
      Mantid::API::IAlgorithm_sptr savePks_alg =
          createChildAlgorithm("SaveIsawPeaks");
      savePks_alg->setPropertyValue("InputWorkspace", runWS[i_run]->getName());
      savePks_alg->setProperty("Filename", outputdir + "ls" +
                                               runWS[i_run]->getName() +
                                               ".integrate");
      savePks_alg->executeAsChildAlg();
      g_log.notice() << "See output file: " << outputdir + "ls" +
                                                   runWS[i_run]->getName() +
                                                   ".integrate"
                     << "\n";
      // Save UB
      Mantid::API::IAlgorithm_sptr saveUB_alg =
          createChildAlgorithm("SaveIsawUB");
      saveUB_alg->setPropertyValue("InputWorkspace", runWS[i_run]->getName());
      saveUB_alg->setProperty("Filename", outputdir + "ls" +
                                              runWS[i_run]->getName() + ".mat");
      saveUB_alg->executeAsChildAlg();
      // Show the names of files written
      g_log.notice() << "See output file: "
                     << outputdir + "ls" + runWS[i_run]->getName() + ".mat"
                     << "\n";
    }
  }
}
//-----------------------------------------------------------------------------------------
/**
  @param  cell_type    cell type to optimize
  @param  lat             optimized cell parameters
  @return  fun_str       string of parameters for fitting
*/
std::string OptimizeLatticeForCellType::inParams(std::string cell_type,
                                                 std::vector<double> &lat) {
  std::ostringstream fun_str;
  fun_str << "name=LatticeFunction,CrystalSystem=" << cell_type;

  std::vector<double> lattice_parameters;
  lattice_parameters.assign(6, 0);
  if (cell_type == ReducedCell::CUBIC()) {
    fun_str << ",a=" << lat[0];
  } else if (cell_type == ReducedCell::TETRAGONAL()) {
    fun_str << ",a=" << lat[0];
    fun_str << ",c=" << lat[2];
  } else if (cell_type == ReducedCell::ORTHORHOMBIC()) {
    fun_str << ",a=" << lat[0];
    fun_str << ",b=" << lat[1];
    fun_str << ",c=" << lat[2];
  } else if (cell_type == "Trigonal") {
    fun_str << ",a=" << lat[0];
    fun_str << ",Alpha=" << lat[3];
  } else if (cell_type == ReducedCell::HEXAGONAL()) {
    fun_str << ",a=" << lat[0];
    fun_str << ",c=" << lat[2];
  } else if (cell_type == ReducedCell::MONOCLINIC()) {
    fun_str << ",a=" << lat[0];
    fun_str << ",b=" << lat[1];
    fun_str << ",c=" << lat[2];
    fun_str << ",Beta=" << lat[4];
  } else if (cell_type == ReducedCell::TRICLINIC()) {
    fun_str << ",a=" << lat[0];
    fun_str << ",b=" << lat[1];
    fun_str << ",c=" << lat[2];
    fun_str << ",Alpha=" << lat[3];
    fun_str << ",Beta=" << lat[4];
    fun_str << ",Gamma=" << lat[5];
  }

  return fun_str.str();
}
//-----------------------------------------------------------------------------------------
/**
  @param  cell_type           cell type to optimize
  @param  col                     1 for parameter 2 for error
  @param  ParamTable       optimized cell parameters
  @return  chisq of optimization
*/
std::vector<double>
OptimizeLatticeForCellType::outParams(std::string cell_type, int icol,
                                      ITableWorkspace_sptr ParamTable) {
  std::vector<double> lattice_parameters;
  lattice_parameters.assign(6, 0);

  if (cell_type == ReducedCell::CUBIC()) {
    lattice_parameters[0] = ParamTable->Double(0, icol);
    lattice_parameters[1] = ParamTable->Double(0, icol);
    lattice_parameters[2] = ParamTable->Double(0, icol);
    if (icol == 2)
      return lattice_parameters;
    lattice_parameters[3] = 90;
    lattice_parameters[4] = 90;
    lattice_parameters[5] = 90;
  } else if (cell_type == ReducedCell::TETRAGONAL()) {
    lattice_parameters[0] = ParamTable->Double(0, icol);
    lattice_parameters[1] = ParamTable->Double(0, icol);
    lattice_parameters[2] = ParamTable->Double(1, icol);
    if (icol == 2)
      return lattice_parameters;
    lattice_parameters[3] = 90;
    lattice_parameters[4] = 90;
    lattice_parameters[5] = 90;
  } else if (cell_type == ReducedCell::ORTHORHOMBIC()) {
    lattice_parameters[0] = ParamTable->Double(0, icol);
    lattice_parameters[1] = ParamTable->Double(1, icol);
    lattice_parameters[2] = ParamTable->Double(2, icol);
    if (icol == 2)
      return lattice_parameters;
    lattice_parameters[3] = 90;
    lattice_parameters[4] = 90;
    lattice_parameters[5] = 90;
  } else if (cell_type == "Trigonal") {
    lattice_parameters[0] = ParamTable->Double(0, icol);
    lattice_parameters[1] = ParamTable->Double(0, icol);
    lattice_parameters[2] = ParamTable->Double(0, icol);
    lattice_parameters[3] = ParamTable->Double(1, icol);
    lattice_parameters[4] = ParamTable->Double(1, icol);
    lattice_parameters[5] = ParamTable->Double(1, icol);
  } else if (cell_type == ReducedCell::HEXAGONAL()) {
    lattice_parameters[0] = ParamTable->Double(0, icol);
    lattice_parameters[1] = ParamTable->Double(0, icol);
    lattice_parameters[2] = ParamTable->Double(1, icol);
    if (icol == 2)
      return lattice_parameters;
    lattice_parameters[3] = 90;
    lattice_parameters[4] = 90;
    lattice_parameters[5] = 120;
  } else if (cell_type == ReducedCell::MONOCLINIC()) {
    lattice_parameters[0] = ParamTable->Double(0, icol);
    lattice_parameters[1] = ParamTable->Double(1, icol);
    lattice_parameters[2] = ParamTable->Double(2, icol);
    lattice_parameters[4] = ParamTable->Double(3, icol);
    if (icol == 2)
      return lattice_parameters;
    lattice_parameters[3] = 90;
    lattice_parameters[5] = 90;
  } else if (cell_type == ReducedCell::TRICLINIC()) {
    lattice_parameters[0] = ParamTable->Double(0, icol);
    lattice_parameters[1] = ParamTable->Double(1, icol);
    lattice_parameters[2] = ParamTable->Double(2, icol);
    lattice_parameters[3] = ParamTable->Double(3, icol);
    lattice_parameters[4] = ParamTable->Double(4, icol);
    lattice_parameters[5] = ParamTable->Double(5, icol);
  }

  return lattice_parameters;
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

    if (col < Edge || col >= (RDet->xpixels() - Edge) || row < Edge ||
        row >= (RDet->ypixels() - Edge))
      return true;
    else
      return false;
  } else {
    std::vector<Geometry::IComponent_const_sptr> children;
    boost::shared_ptr<const Geometry::ICompAssembly> asmb =
        boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(parent);
    asmb->getChildren(children, false);
    boost::shared_ptr<const Geometry::ICompAssembly> asmb2 =
        boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(children[0]);
    std::vector<Geometry::IComponent_const_sptr> grandchildren;
    asmb2->getChildren(grandchildren, false);
    int NROWS = static_cast<int>(grandchildren.size());
    int NCOLS = static_cast<int>(children.size());
    // Wish pixels and tubes start at 1 not 0
    if (col - 1 < Edge || col - 1 >= (NCOLS - Edge) || row - 1 < Edge ||
        row - 1 >= (NROWS - Edge))
      return true;
    else
      return false;
  }
  return false;
}

} // namespace Algorithm
} // namespace Mantid

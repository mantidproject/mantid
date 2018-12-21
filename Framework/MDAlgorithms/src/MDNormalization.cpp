// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidMDAlgorithms/MDNormalization.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Sample.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"
#include "MantidGeometry/Crystal/SpaceGroupFactory.h"
#include "MantidGeometry/Crystal/PointGroupFactory.h"
#include <boost/lexical_cast.hpp>
#include "MantidKernel/Exception.h"

namespace Mantid {
namespace MDAlgorithms {

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MDNormalization)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string MDNormalization::name() const { return "MDNormalization"; }

/// Algorithm's version for identification. @see Algorithm::version
int MDNormalization::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string MDNormalization::category() const {
  return "MDAlgorithms\\Normalisation";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string MDNormalization::summary() const {
  return "Bins multidimensional data and calculate the normalization on the same grid";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void MDNormalization::init() {
  declareProperty(make_unique<WorkspaceProperty<API::IMDEventWorkspace>>(
                       "InputWorkspace", "", Kernel::Direction::Input),
                  "An input MDEventWorkspace. Must be in Q_sample frame.");

  // RLU and settings
  declareProperty("RLU", true, "Use reciprocal lattice units. If false, use Q_sample");
  setPropertyGroup("RLU","Q projections RLU");

  auto mustBe3D = boost::make_shared<Kernel::ArrayLengthValidator<double> >(3);
  std::vector<double> Q1(3, 0.), Q2(3, 0), Q3(3, 0);
  Q1[0] = 1.;
  Q2[1] = 1.;
  Q3[2] = 1.;

  declareProperty(make_unique<ArrayProperty<double>>("QDimension1", Q1, mustBe3D),
                  "The first Q projection axis - Default is (1,0,0)");
  setPropertySettings("QDimension1", make_unique<Kernel::VisibleWhenProperty>("RLU", IS_EQUAL_TO, "1"));
  setPropertyGroup("QDimension1","Q projections RLU");

  declareProperty(make_unique<ArrayProperty<double>>("QDimension2", Q2, mustBe3D),
                  "The second Q projection axis - Default is (0,1,0)");
  setPropertySettings("QDimension2", make_unique<Kernel::VisibleWhenProperty>("RLU", IS_EQUAL_TO, "1"));
  setPropertyGroup("QDimension2","Q projections RLU");

  declareProperty(make_unique<ArrayProperty<double>>("QDimension3", Q3, mustBe3D),
                  "The thirdtCalculateCover Q projection axis - Default is (0,0,1)");
  setPropertySettings("QDimension3", make_unique<Kernel::VisibleWhenProperty>("RLU", IS_EQUAL_TO, "1"));
  setPropertyGroup("QDimension3","Q projections RLU");

  // vanadium
  auto fluxValidator = boost::make_shared<CompositeValidator>();
  fluxValidator->add<InstrumentValidator>();
  fluxValidator->add<CommonBinsValidator>();
  auto solidAngleValidator = fluxValidator->clone();
  declareProperty(make_unique<WorkspaceProperty<>>("SolidAngleWorkspace", "",
                                                   Direction::Input, API::PropertyMode::Optional,
                                                   solidAngleValidator),
                  "An input workspace containing integrated vanadium "
                  "(a measure of the solid angle).\n"
                  "Mandatory for diffraction, optional for direct geometry inelastic");
  declareProperty(make_unique<WorkspaceProperty<>>(
                      "FluxWorkspace", "", Direction::Input, API::PropertyMode::Optional,
                      fluxValidator),
                  "An input workspace containing momentum dependent flux.\n"
                  "Mandatory for diffraction. No effect on direct geometry inelastic");
  setPropertyGroup("SolidAngleWorkspace","Vanadium normalization");
  setPropertyGroup("FluxWorkspace","Vanadium normalization");

  // Define slicing
  for(std::size_t i=0;i<6;i++) {
    std::string propName = "Dimension"+Strings::toString(i)+"Name";
    std::string propBinning = "Dimension"+Strings::toString(i)+"Binning";
    declareProperty(Kernel::make_unique<PropertyWithValue<std::string>>(propName, "", Direction::Input),
                    "Name for the " + Strings::toString(i) + "th dimension. Leave blank for NONE.");
    auto atMost3 = boost::make_shared<ArrayLengthValidator<double> >(0,3);
    std::vector<double> temp;
    declareProperty(Kernel::make_unique<ArrayProperty<double>>(propBinning,temp,atMost3),
                    "Binning for the " + Strings::toString(i) + "th dimension.\n"+
                    "- Leave blank for complete integration\n"+
                    "- One value is interpreted as step\n"
                    "- Two values are interpreted integration interval\n"+
                    "- Three values are interpreted as min, step, max");
    setPropertyGroup(propName, "Binning");
    setPropertyGroup(propBinning, "Binning");
  }

  // symmetry operations
  declareProperty(Kernel::make_unique<PropertyWithValue<std::string>>("SymmetryOperations", "", Direction::Input),
                  "If specified the symmetry will be applied, "
                  "can be space group name, point group name, or list individual symmetries.");

  // temporary workspaces
  declareProperty(make_unique<WorkspaceProperty<IMDHistoWorkspace>>(
                      "TemporaryDataWorkspace", "", Direction::Input,
                      PropertyMode::Optional),
                  "An input MDHistoWorkspace used to accumulate data from "
                  "multiple MDEventWorkspaces. If unspecified a blank "
                  "MDHistoWorkspace will be created.");
  declareProperty(make_unique<WorkspaceProperty<IMDHistoWorkspace>>(
                      "TemporaryNormalizationWorkspace", "", Direction::Input,
                      PropertyMode::Optional),
                  "An input MDHistoWorkspace used to accumulate normalization "
                  "from multiple MDEventWorkspaces. If unspecified a blank "
                  "MDHistoWorkspace will be created.");
  setPropertyGroup("TemporaryDataWorkspace", "Temporary workspaces");
  setPropertyGroup("TemporaryNormalizationWorkspace", "Temporary workspaces");

  declareProperty(make_unique<WorkspaceProperty<API::Workspace>>(
                      "OutputWorkspace", "", Kernel::Direction::Output),
                  "A name for the output data MDHistoWorkspace.");
  declareProperty(make_unique<WorkspaceProperty<Workspace>>(
                      "OutputNormalizationWorkspace", "", Direction::Output),
                  "A name for the output normalization MDHistoWorkspace.");
}

//----------------------------------------------------------------------------------------------
/// Validate the input workspace @see Algorithm::validateInputs
std::map<std::string, std::string>
MDNormalization::validateInputs() {
  std::map<std::string, std::string> errorMessage;

  // Check for input workspace frame
  Mantid::API::IMDEventWorkspace_sptr inputWS =
      this->getProperty("InputWorkspace");
  if (inputWS->getNumDims() < 3) {
    errorMessage.emplace("InputWorkspace",
                         "The input workspace must be at least 3D");
  } else {
    for (size_t i = 0; i < 3; i++) {
      if (inputWS->getDimension(i)->getMDFrame().name() !=
          Mantid::Geometry::QSample::QSampleName) {
        errorMessage.emplace("InputWorkspace",
                             "The input workspace must be in Q_sample");
      }
    }
  }
  // Check if the vanadium is available for diffraction
  bool diffraction=true;
  if ((inputWS->getNumDims() >3) && (inputWS->getDimension(3)->getMDFrame().name()=="DeltaE")) {
    diffraction = false;
  }
  if (diffraction) {
    API::MatrixWorkspace_const_sptr solidAngleWS = getProperty("SolidAngleWorkspace");
    API::MatrixWorkspace_const_sptr fluxWS = getProperty("FluxWorkspace");
    if (solidAngleWS == nullptr) {
      errorMessage.emplace("SolidAngleWorkspace","SolidAngleWorkspace is required for diffraction");
    }
    if (fluxWS == nullptr) {
      errorMessage.emplace("FluxWorkspace","FluxWorkspace is required for diffraction");
    }
  }
  // Check for property MDNorm_low and MDNorm_high
  size_t nExperimentInfos = inputWS->getNumExperimentInfo();
  if (nExperimentInfos == 0) {
    errorMessage.emplace("InputWorkspace",
                         "There must be at least one experiment info");
  } else {
    for (size_t iExpInfo = 0; iExpInfo < nExperimentInfos; iExpInfo++) {
      auto &currentExptInfo =
          *(inputWS->getExperimentInfo(static_cast<uint16_t>(iExpInfo)));
      if (!currentExptInfo.run().hasProperty("MDNorm_low")) {
        errorMessage.emplace("InputWorkspace", "Missing MDNorm_low log. Please "
                                               "use CropWorkspaceForMDNorm "
                                               "before converting to MD");
      }
      if (!currentExptInfo.run().hasProperty("MDNorm_high")) {
        errorMessage.emplace("InputWorkspace",
                             "Missing MDNorm_high log. Please use "
                             "CropWorkspaceForMDNorm before converting to MD");
      }
    }
  }
  // check projections and UB
  if (getProperty("RLU")) {
    DblMatrix W = DblMatrix(3, 3);
    std::vector<double> Q1Basis = getProperty("QDimension1");
    std::vector<double> Q2Basis = getProperty("QDimension2");
    std::vector<double> Q3Basis = getProperty("QDimension3");
    W.setColumn(0, Q1Basis);
    W.setColumn(1, Q2Basis);
    W.setColumn(2, Q3Basis);
    if (fabs(W.determinant()) < 1e-5) {
      errorMessage.emplace("QDimension1",
                           "The projection dimensions are coplanar or zero");
      errorMessage.emplace("QDimension2",
                           "The projection dimensions are coplanar or zero");
      errorMessage.emplace("QDimension3",
                           "The projection dimensions are coplanar or zero");
    }
    if (!inputWS->getExperimentInfo(0)->sample().hasOrientedLattice()) {
      errorMessage.emplace("InputWorkspace", "There is no oriented lattice "
                                             "associated with the input workspace. "
                                             "Use SetUB algorithm");
    }
  }
  // check dimension names
  std::vector<std::string> originalDimensionNames;
  for (size_t i=3; i<inputWS->getNumDims(); i++) {
    originalDimensionNames.push_back(inputWS->getDimension(i)->getName());
  }
  originalDimensionNames.push_back("QDimension1");
  originalDimensionNames.push_back("QDimension2");
  originalDimensionNames.push_back("QDimension3");
  std::vector<std::string> selectedDimensions;
  for(std::size_t i=0;i<6;i++) {
    std::string propName = "Dimension"+Strings::toString(i)+"Name";
    std::string dimName = getProperty(propName);
    std::string binningName = "Dimension"+Strings::toString(i)+"Binning";
    std::vector<double> binning = getProperty(binningName);
    if (!dimName.empty()) {
      auto it = std::find(originalDimensionNames.begin(),originalDimensionNames.end(),dimName);
      if (it==originalDimensionNames.end()) {
        errorMessage.emplace(propName, "Name '"+dimName+"' is not one of the "
                             "original workspace names or a Q dimension");
      } else {
        //make sure dimension is unique
        auto itSel = std::find(selectedDimensions.begin(),selectedDimensions.end(),dimName);
        if (itSel==selectedDimensions.end()){
          selectedDimensions.push_back(dimName);
        } else{
          errorMessage.emplace(propName, "Name '"+dimName+"' was already selected");
        }
      }
      if (binning.empty()) {
        errorMessage.emplace(binningName, "Define binning if the dimension name is not empty");
      }
    } else {
      if (!binning.empty()) {
        errorMessage.emplace(binningName, "There should be no binning if the dimension name is empty");
      }
    }
  }
  // since Q dimensions can be non - orthogonal, all must be present
  if ((std::find(selectedDimensions.begin(),selectedDimensions.end(),"QDimension1") == selectedDimensions.end())||
      (std::find(selectedDimensions.begin(),selectedDimensions.end(),"QDimension2") == selectedDimensions.end())||
      (std::find(selectedDimensions.begin(),selectedDimensions.end(),"QDimension3") == selectedDimensions.end())) {
    for(std::size_t i=0;i<6;i++) {
        std::string propName = "Dimension"+Strings::toString(i)+"Name";
        errorMessage.emplace(propName, "All of QDimension1, QDimension2, QDimension3 must be present");
    }
  }
  // symmetry operations
  std::string symOps = this->getProperty("SymmetryOperations");
  if(!symOps.empty()) {
    bool isSpaceGroup =Geometry::SpaceGroupFactory::Instance().isSubscribed(symOps);
    bool isPointGroup = Geometry::PointGroupFactory::Instance().isSubscribed(symOps);
    if(!isSpaceGroup &&!isPointGroup) {
        try {
          Geometry::SymmetryOperationFactory::Instance().createSymOps(symOps);
        }
        catch (const Mantid::Kernel::Exception::ParseError&) {
          errorMessage.emplace("SymmetryOperations", "The input is not a space group, a point group, or a list of symmetry operations");
        }
    }
  }
  return errorMessage;
}


//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void MDNormalization::exec() {

  // symmetry operations
  std::string symOps = this->getProperty("SymmetryOperations");
  std::vector<Geometry::SymmetryOperation> symmetryOps;
  if (symOps.empty()){
    symOps="x,y,z";
  }
  if(Geometry::SpaceGroupFactory::Instance().isSubscribed(symOps)){
    auto spaceGroup = Geometry::SpaceGroupFactory::Instance().createSpaceGroup(symOps);
    auto pointGroup = spaceGroup->getPointGroup();
    symmetryOps = pointGroup->getSymmetryOperations();
  } else if (Geometry::PointGroupFactory::Instance().isSubscribed(symOps)) {
    auto pointGroup = Geometry::PointGroupFactory::Instance().createPointGroup(symOps);
    symmetryOps = pointGroup->getSymmetryOperations();
  } else {
    symmetryOps = Geometry::SymmetryOperationFactory::Instance().createSymOps(symOps);
  }
  g_log.debug()<<"Symmetry operations\n";
  for(auto so:symmetryOps){
    g_log.debug()<<so.identifier()<<"\n";
  }

  //get the workspaces
  Mantid::API::IMDEventWorkspace_sptr inputWS =
        this->getProperty("InputWorkspace");
  Mantid::API::IMDHistoWorkspace_sptr tempDataWS =
        this->getProperty("TemporaryDataWorkspace");
  Mantid::API::Workspace_sptr outputWS = this->getProperty("OutputWorkspace");

  std::vector<std::string> originalDimensionNames;
  originalDimensionNames.push_back("QDimension1");
  originalDimensionNames.push_back("QDimension2");
  originalDimensionNames.push_back("QDimension3");
  for (size_t i=3; i<inputWS->getNumDims(); i++) {
    originalDimensionNames.push_back(inputWS->getDimension(i)->getName());
  }

  // get binning dimensions
  std::vector<std::string> binDimensionNames;
  std::vector<std::vector<double>> binDimensionBinning;

  for(std::size_t i=0;i<6;i++) {
    std::string propName = "Dimension"+Strings::toString(i)+"Name";
    std::string binningName = "Dimension"+Strings::toString(i)+"Binning";
    std::string dimName = getProperty(propName);
    std::vector<double> binning = getProperty(binningName);
    if (!dimName.empty()) {
      binDimensionNames.push_back(dimName);
      binDimensionBinning.push_back(binning);
    }
  }


  bool isRLU = getProperty("RLU");


  std::vector<double> Q1Basis{1., 0., 0.}, Q2Basis{0., 1., 0.},Q3Basis{0., 0., 1.};
  DblMatrix UB;
  if (isRLU) {
    Q1Basis = getProperty("QDimension1");
    Q2Basis = getProperty("QDimension2");
    Q3Basis = getProperty("QDimension3");
    UB = inputWS->getExperimentInfo(0)->sample().getOrientedLattice().getUB()*2*M_PI;
  }
  double soIndex = 0;
  for(auto so:symmetryOps){
    // calculate dimensions for binning
    V3D Q1,Q2,Q3;
    Q1=so.transformHKL(V3D(Q1Basis[0],Q1Basis[1],Q1Basis[2]));
    Q2=so.transformHKL(V3D(Q2Basis[0],Q2Basis[1],Q2Basis[2]));
    Q3=so.transformHKL(V3D(Q3Basis[0],Q3Basis[1],Q3Basis[2]));

    if (isRLU) {
      Q1 = UB * Q1;
      Q2 = UB * Q2;
      Q3 = UB * Q3;
    }

    // bin the data
    IAlgorithm_sptr binMD = createChildAlgorithm("BinMD", soIndex*0.3, (soIndex+1)*0.3);
    binMD->setPropertyValue("AxisAligned", "0");
    binMD->setProperty("InputWorkspace",inputWS);
    binMD->setProperty("TemporaryDataWorkspace", tempDataWS);
    binMD->setPropertyValue("NormalizeBasisVectors",0);
    binMD->setProperty("OutputWorkspace",outputWS);
    //set binning properties
    for(size_t i=0;i<binDimensionNames.size();i++){
      std::string basisVector;

    }

    //execute algorithm
    binMD->executeAsChildAlg();
    // set the temporary workspace to be the output workspace, so it keeps adding different symmetries
    soIndex+=1;
  }
  this->setProperty("OutputWorkspace",outputWS);
}

} // namespace MDAlgorithms
} // namespace Mantid

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
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidGeometry/MDGeometry/HKL.h"
#include "MantidGeometry/MDGeometry/MDFrameFactory.h"
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
using namespace Mantid::DataObjects;

static bool abs_compare(int a, int b) {
    return (std::abs(a) < std::abs(b));
}
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MDNormalization)

//----------------------------------------------------------------------------------------------
/**
 * Constructor
 */
MDNormalization::MDNormalization()
    :m_normWS(), m_inputWS(), m_isRLU(false), m_UB(3,3), m_W(3,3,true), m_transformation(),
      m_numExptInfos(0), m_diffraction(true), m_accumulate(false), m_dEIntegrated(false) {}


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

  m_isRLU = getProperty("RLU");
  //get the workspaces
  m_inputWS = this->getProperty("InputWorkspace");
  if ((m_inputWS->getNumDims() >3) && (m_inputWS->getDimension(3)->getMDFrame().name()=="DeltaE")) {
    m_diffraction = false;
  }
  auto outputWS = binInputWS(symmetryOps);

  createNormalizationWS(*outputWS);
  this->setProperty("OutputNormalizationWorkspace",m_normWS);
  this->setProperty("OutputWorkspace",outputWS);

  m_numExptInfos = outputWS->getNumExperimentInfo();
  // loop over all experiment infos
  for (uint16_t expInfoIndex = 0; expInfoIndex < m_numExptInfos;
       expInfoIndex++) {
    // Check for other dimensions if we could measure anything in the original
    // data
    bool skipNormalization = false;
    const std::vector<coord_t> otherValues =
        getValuesFromOtherDimensions(skipNormalization, expInfoIndex);
    g_log.warning()<<skipNormalization<<"\n";
   /*
    const auto affineTrans =
        findIntergratedDimensions(otherValues, skipNormalization);
    cacheDimensionXValues();

    if (!skipNormalization) {
      calculateNormalization(otherValues, affineTrans, expInfoIndex);
    } else {
      g_log.warning("Binning limits are outside the limits of the MDWorkspace. "
                    "Not applying normalization.");
    }
    */
    // if more than one experiment info, keep accumulating
    m_accumulate = true;
  }
}



std::string MDNormalization::QDimensionName(std::vector<double> projection) {
  std::vector<double>::iterator result;
  result = std::max_element(projection.begin(), projection.end(), abs_compare);
  std::vector<char> symbol{'H','K','L'};
  char character=symbol[std::distance(projection.begin(), result)];
  std::stringstream name;
  name<<"[";
  for(size_t i=0;i<3;i++){
    if(projection[i]==0){
      name<<"0";
    } else if (projection[i]==1){
      name<<character;
    } else if (projection[i]==-1){
      name<<"-"<<character;
    } else {
      name<<std::defaultfloat<<std::setprecision(3)<<projection[i]<<character;
    }
    if(i!=2){
      name<<",";
    }
  }
  name<<"]";
  return name.str();
}

std::map<std::string, std::string> MDNormalization::getBinParameters()
{
    std::map<std::string, std::string> parameters;
    std::stringstream extents;
    std::stringstream bins;
    std::vector<std::string> originalDimensionNames;
    originalDimensionNames.push_back("QDimension1");
    originalDimensionNames.push_back("QDimension2");
    originalDimensionNames.push_back("QDimension3");
    for (size_t i=3; i<m_inputWS->getNumDims(); i++) {
      originalDimensionNames.push_back(m_inputWS->getDimension(i)->getName());
    }

    if (m_isRLU) {
      m_Q1Basis = getProperty("QDimension1");
      m_Q2Basis = getProperty("QDimension2");
      m_Q3Basis = getProperty("QDimension3");
      m_UB = m_inputWS->getExperimentInfo(0)->sample().getOrientedLattice().getUB()*2*M_PI;
    }

    std::vector<double> W(m_Q1Basis);
    W.insert(W.end(),m_Q2Basis.begin(),m_Q2Basis.end());
    W.insert(W.end(),m_Q3Basis.begin(),m_Q3Basis.end());
    m_W = DblMatrix(W);
    m_W.Transpose();

    // Find maximum Q
    auto &exptInfo0 = *(m_inputWS->getExperimentInfo(static_cast<uint16_t>(0)));
    auto upperLimitsVector = (*(dynamic_cast<Kernel::PropertyWithValue<std::vector<double>> *>(
                exptInfo0.getLog("MDNorm_high"))))();
    double maxQ;
    if(m_diffraction){
      maxQ=2.*(*std::max_element(upperLimitsVector.begin(),upperLimitsVector.end()));
    } else {
      double Ei;
      double maxDE = *std::max_element(upperLimitsVector.begin(),upperLimitsVector.end());
      auto loweLimitsVector = (*(dynamic_cast<Kernel::PropertyWithValue<std::vector<double>> *>(
                  exptInfo0.getLog("MDNorm_low"))))();
      double minDE = *std::min_element(loweLimitsVector.begin(),loweLimitsVector.end());
      if (exptInfo0.run().hasProperty("Ei")) {
        Kernel::Property *eiprop = exptInfo0.run().getProperty("Ei");
        Ei = boost::lexical_cast<double>(eiprop->value());
        if (Ei <= 0) {
          throw std::invalid_argument("Ei stored in the workspace is not positive");
        }
      } else {
        throw std::invalid_argument("Could not find Ei value in the workspace.");
      }
      const double energyToK = 8.0 * M_PI * M_PI * PhysicalConstants::NeutronMass *
                               PhysicalConstants::meV * 1e-20 /
                               (PhysicalConstants::h * PhysicalConstants::h);
      double ki = std::sqrt(energyToK * Ei);
      double kfmin = std::sqrt(energyToK * (Ei - minDE));
      double kfmax = std::sqrt(energyToK * (Ei - maxDE));

      maxQ=ki+std::max(kfmin,kfmax);
    }
    size_t basisVectorIndex=0;
    std::vector<double> transformation;
    for(std::size_t i=0;i<6;i++) {
      std::string propName = "Dimension"+Strings::toString(i)+"Name";
      std::string binningName = "Dimension"+Strings::toString(i)+"Binning";
      std::string dimName = getProperty(propName);
      std::vector<double> binning = getProperty(binningName);
      std::string bv="BasisVector";
      if (!dimName.empty()) {
          std::string property=bv+Strings::toString(basisVectorIndex);
          std::stringstream propertyValue;
          propertyValue<<dimName;
          //get the index in the original workspace
          auto dimIndex=std::distance(originalDimensionNames.begin(),std::find(originalDimensionNames.begin(),originalDimensionNames.end(),dimName));
          auto dimension=m_inputWS->getDimension(dimIndex);
          propertyValue<<","<<dimension->getMDUnits().getUnitLabel().ascii();
          for (size_t j=0;j<originalDimensionNames.size();j++){
              if(j==static_cast<size_t>(dimIndex)){
                propertyValue<<",1";
                transformation.push_back(1.);
              } else {
                propertyValue<<",0";
                transformation.push_back(0.);
              }
          }
          parameters.emplace(property,propertyValue.str());
          //get the extents an number of bins
          coord_t dimMax=dimension->getMaximum();
          coord_t dimMin=dimension->getMinimum();
          if(m_isRLU){
             Mantid::Geometry::OrientedLattice ol;
             ol.setUB(m_UB*m_W); //note that this is already multiplied by 2Pi
             if(dimIndex==0) {
                 dimMax=static_cast<coord_t>(ol.a()*maxQ);
                 dimMin=-dimMax;
             } else if (dimIndex==1){
                 dimMax=static_cast<coord_t>(ol.b()*maxQ);
                 dimMin=-dimMax;
             } else if (dimIndex==2){
                 dimMax=static_cast<coord_t>(ol.c()*maxQ);
                 dimMin=-dimMax;
             }
          }
          if (binning.size()==0){
            //only one bin, integrating from min to max
            extents<<dimMin<<","<<dimMax<<",";
            bins<<1<<",";
          } else if (binning.size()==2){
              //only one bin, integrating from min to max
              extents<<binning[0]<<","<<binning[1]<<",";
              bins<<1<<",";
          } else if (binning.size()==1){
            auto step=binning[0];
            int nsteps=static_cast<int>(std::ceil((dimMax-dimMin)/step));
            bins<<nsteps<<",";
            extents<<dimMin<<","<<dimMin+nsteps*step<<",";
          } else if (binning.size()==3){
              dimMin=static_cast<coord_t>(binning[0]);
              auto step=binning[1];
              dimMax=static_cast<coord_t>(binning[2]);
              int nsteps=static_cast<int>(std::ceil((dimMax-dimMin)/step));
              bins<<nsteps<<",";
              extents<<dimMin<<","<<dimMin+nsteps*step<<",";
            }
          basisVectorIndex++;
      }
    }
    parameters.emplace("OutputExtents",extents.str());
    parameters.emplace("OutputBins",bins.str());
    m_transformation = DblMatrix(transformation,static_cast<size_t>((transformation.size())/m_inputWS->getNumDims()), m_inputWS->getNumDims());
    return parameters;
}

void MDNormalization::createNormalizationWS(const DataObjects::MDHistoWorkspace &dataWS)
{
    // Copy the MDHisto workspace, and change signals and errors to 0.
    boost::shared_ptr<IMDHistoWorkspace> tmp =
        this->getProperty("TemporaryNormalizationWorkspace");
    m_normWS = boost::dynamic_pointer_cast<MDHistoWorkspace>(tmp);
    if (!m_normWS) {
      m_normWS = dataWS.clone();
      m_normWS->setTo(0., 0., 0.);
    } else {
      m_accumulate = true;
    }
}

DataObjects::MDHistoWorkspace_sptr MDNormalization::binInputWS(std::vector<Geometry::SymmetryOperation> symmetryOps)
{
    Mantid::API::IMDHistoWorkspace_sptr tempDataWS =
          this->getProperty("TemporaryDataWorkspace");
    Mantid::API::Workspace_sptr outputWS;
    std::map<std::string, std::string> parameters=getBinParameters();
    double soIndex = 0;
    std::vector<size_t> qDimensionIndices;
    for(auto so:symmetryOps){
      // calculate dimensions for binning
      V3D Q1,Q2,Q3;
      Q1=so.transformHKL(V3D(m_Q1Basis[0],m_Q1Basis[1],m_Q1Basis[2]));
      Q2=so.transformHKL(V3D(m_Q2Basis[0],m_Q2Basis[1],m_Q2Basis[2]));
      Q3=so.transformHKL(V3D(m_Q3Basis[0],m_Q3Basis[1],m_Q3Basis[2]));

      if (m_isRLU) {
        Q1 = m_UB * Q1;
        Q2 = m_UB * Q2;
        Q3 = m_UB * Q3;
      }

      // bin the data
      double fraction=1./static_cast<double>(symmetryOps.size());
      IAlgorithm_sptr binMD = createChildAlgorithm("BinMD", soIndex*0.3*fraction, (soIndex+1)*0.3*fraction);
      binMD->setPropertyValue("AxisAligned", "0");
      binMD->setProperty("InputWorkspace",m_inputWS);
      binMD->setProperty("TemporaryDataWorkspace", tempDataWS);
      binMD->setPropertyValue("NormalizeBasisVectors","0");
      binMD->setPropertyValue("OutputWorkspace",getPropertyValue("OutputWorkspace"));
      //set binning properties
      size_t qindex=0;
      for( auto const& p : parameters ) {
        auto key=p.first;
        auto value=p.second;

        std::stringstream basisVector;
        std::vector<double> projection(m_inputWS->getNumDims(),0.);
        if (value.find("QDimension1")!=std::string::npos) {
            if (!m_isRLU) {
              projection[0]=1.;
              basisVector<<"Q_sample_x,A^{-1}";
            } else {
              qDimensionIndices.push_back(qindex);
              projection[0]=Q1.X();
              projection[1]=Q1.Y();
              projection[2]=Q1.Z();
              basisVector<<QDimensionName(m_Q1Basis)<<", r.l.u.";
            }
        } else if (value.find("QDimension2")!=std::string::npos) {
            if (!m_isRLU) {
              projection[1]=1.;
              basisVector<<"Q_sample_y,A^{-1}";
            } else {
              qDimensionIndices.push_back(qindex);
              projection[0]=Q2.X();
              projection[1]=Q2.Y();
              projection[2]=Q2.Z();
              basisVector<<QDimensionName(m_Q2Basis)<<", r.l.u.";
            }
        } else if (value.find("QDimension3")!=std::string::npos) {
            if (!m_isRLU) {
              projection[2]=1.;
              basisVector<<"Q_sample_z,A^{-1}";
            } else {
              qDimensionIndices.push_back(qindex);
              projection[0]=Q3.X();
              projection[1]=Q3.Y();
              projection[2]=Q3.Z();
              basisVector<<QDimensionName(m_Q3Basis)<<", r.l.u.";
            }
        }
        if (!basisVector.str().empty()){
            for(auto const& proji: projection){
                basisVector<<","<<proji;
            }
            value=basisVector.str();
        }
        g_log.debug()<<"Binning parameter "<<key<<" value: "<<value<<"\n";
        binMD->setPropertyValue(key, value);
        qindex++;
      }
      //execute algorithm
      binMD->executeAsChildAlg();
      outputWS=binMD->getProperty("OutputWorkspace");

      //set the temporary workspace to be the output workspace, so it keeps adding different symmetries
      tempDataWS = boost::dynamic_pointer_cast<MDHistoWorkspace>(outputWS);
      soIndex+=1;
    }

    auto outputMDHWS = boost::dynamic_pointer_cast<MDHistoWorkspace>(outputWS);
    //set MDUnits for Q dimensions
    if(m_isRLU){
      Mantid::Geometry::MDFrameArgument argument(Mantid::Geometry::HKL::HKLName,"r.l.u.");
      auto mdFrameFactory = Mantid::Geometry::makeMDFrameFactoryChain();
      Mantid::Geometry::MDFrame_uptr hklFrame =  mdFrameFactory->create(argument);
      for(size_t i:qDimensionIndices){
        auto mdHistoDimension =
                boost::const_pointer_cast<Mantid::Geometry::MDHistoDimension>(
                    boost::dynamic_pointer_cast<
                        const Mantid::Geometry::MDHistoDimension>(outputMDHWS->getDimension(i)));
        mdHistoDimension->setMDFrame(*hklFrame);
      }
    }

    outputMDHWS->setDisplayNormalization(Mantid::API::NoNormalization);
    return outputMDHWS;
}

std::vector<coord_t> MDNormalization::getValuesFromOtherDimensions(bool &skipNormalization, uint16_t expInfoIndex) const
{
    const auto &currentRun = m_inputWS->getExperimentInfo(expInfoIndex)->run();

    std::vector<coord_t> otherDimValues;
    for (size_t i = 3; i < m_inputWS->getNumDims(); i++) {
      const auto dimension = m_inputWS->getDimension(i);
      coord_t inputDimMin = static_cast<float>(dimension->getMinimum());
      coord_t inputDimMax = static_cast<float>(dimension->getMaximum());
      coord_t outputDimMin(0),outputDimMax(0);
      bool isIntegrated=true;

      for(size_t j=0; j<m_transformation.numRows(); j++) {
          if(m_transformation[j][i]==1){
              isIntegrated = false;
              outputDimMin=m_normWS->getDimension(j)->getMinimum();
              outputDimMax=m_normWS->getDimension(j)->getMaximum();
          }
      }
      if(dimension->getName()=="DeltaE") {
          if((inputDimMax<outputDimMin) || (inputDimMin>outputDimMax)){
              skipNormalization=true;
          }
      } else {
        coord_t value = static_cast<coord_t>(currentRun.getLogAsSingleValue(
                        dimension->getName(), Mantid::Kernel::Math::TimeAveragedMean));
        otherDimValues.push_back(value);
        if (value < inputDimMin || value > inputDimMax) {
            skipNormalization = true;
        }
        if ((!isIntegrated) && (value < outputDimMin || value > outputDimMax)) {
            skipNormalization = true;
        }
      }
    }
    return otherDimValues;
}

} // namespace MDAlgorithms
} // namespace Mantid

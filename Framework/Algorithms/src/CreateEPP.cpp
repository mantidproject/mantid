// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CreateEPP.h"

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/UnitConversion.h"

namespace Mantid {
namespace Algorithms {

using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

namespace {
/** A private namespace holding the property names.
 */
namespace PropertyNames {
const static std::string INPUT_WORKSPACE("InputWorkspace");
const static std::string OUTPUT_WORKSPACE("OutputWorkspace");
const static std::string SIGMA("Sigma");
} // namespace PropertyNames

/** A private namespace holding the column names of the EPP table.
 */
namespace ColumnNames {
const static std::string WS_INDEX("WorkspaceIndex");
const static std::string PEAK_CENTRE("PeakCentre");
const static std::string PEAK_CENTRE_ERR("PeakCentreError");
const static std::string SIGMA("Sigma");
const static std::string SIGMA_ERR("SigmaError");
const static std::string HEIGHT("Height");
const static std::string HEIGHT_ERR("HeightError");
const static std::string CHI_SQUARED("chiSq");
const static std::string STATUS("FitStatus");
} // namespace ColumnNames

/** Add standard EPP table columns to the input workspace.
 *
 * @param ws The TableWorkspace to add the columns to.
 */
void addEPPColumns(API::ITableWorkspace_sptr ws) {
  ws->addColumn("int", ColumnNames::WS_INDEX);
  ws->addColumn("double", ColumnNames::PEAK_CENTRE);
  ws->addColumn("double", ColumnNames::PEAK_CENTRE_ERR);
  ws->addColumn("double", ColumnNames::SIGMA);
  ws->addColumn("double", ColumnNames::SIGMA_ERR);
  ws->addColumn("double", ColumnNames::HEIGHT);
  ws->addColumn("double", ColumnNames::HEIGHT_ERR);
  ws->addColumn("double", ColumnNames::CHI_SQUARED);
  ws->addColumn("str", ColumnNames::STATUS);
}

} // anonymous namespace

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateEPP)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CreateEPP::name() const { return "CreateEPP"; }

/// Algorithm's version for identification. @see Algorithm::version
int CreateEPP::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CreateEPP::category() const { return "Utility"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CreateEPP::summary() const {
  return "Creates a nominal EPP table compatible with what is returned by the "
         "FindEPP algorithm.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CreateEPP::init() {
  auto inputWSValidator = boost::make_shared<Kernel::CompositeValidator>();
  inputWSValidator->add(boost::make_shared<API::InstrumentValidator>());
  inputWSValidator->add(boost::make_shared<API::WorkspaceUnitValidator>("TOF"));
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input, inputWSValidator),
                  "An input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<API::ITableWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The calculated output EPP table.");
  auto mustBePositive = boost::make_shared<Kernel::BoundedValidator<double>>();
  mustBePositive->setLower(0);
  declareProperty(PropertyNames::SIGMA, 0.0, mustBePositive,
                  "The value to fill the Sigma column with.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CreateEPP::exec() {
  API::MatrixWorkspace_sptr inputWS =
      getProperty(PropertyNames::INPUT_WORKSPACE);
  const auto &spectrumInfo = inputWS->spectrumInfo();
  API::ITableWorkspace_sptr outputWS =
      boost::make_shared<DataObjects::TableWorkspace>();
  addEPPColumns(outputWS);
  const double sigma = getProperty(PropertyNames::SIGMA);
  const size_t spectraCount = spectrumInfo.size();
  outputWS->setRowCount(spectraCount);
  const auto l1 = spectrumInfo.l1();
  const double EFixed = inputWS->run().getPropertyAsSingleValue("Ei");
  for (size_t i = 0; i < spectraCount; ++i) {
    const auto l2 = spectrumInfo.l2(i);
    const auto elasticTOF = Kernel::UnitConversion::run(
        "Energy", "TOF", EFixed, l1, l2, 0, Kernel::DeltaEMode::Direct, EFixed);
    outputWS->getRef<int>(ColumnNames::WS_INDEX, i) = static_cast<int>(i);
    outputWS->getRef<double>(ColumnNames::PEAK_CENTRE, i) = elasticTOF;
    outputWS->getRef<double>(ColumnNames::PEAK_CENTRE_ERR, i) = 0;
    outputWS->getRef<double>(ColumnNames::SIGMA, i) = sigma;
    outputWS->getRef<double>(ColumnNames::SIGMA_ERR, i) = 0;
    double height = 0;
    try {
      const auto elasticIndex = inputWS->yIndexOfX(elasticTOF, i);
      height = inputWS->y(i)[elasticIndex];
    } catch (std::out_of_range &) {
      std::ostringstream sout;
      sout << "EPP out of TOF range for workspace index " << i
           << ". Peak height set to zero.";
      g_log.warning() << sout.str();
    }
    outputWS->getRef<double>(ColumnNames::HEIGHT, i) = height;
    outputWS->getRef<double>(ColumnNames::CHI_SQUARED, i) = 1;
    outputWS->getRef<std::string>(ColumnNames::STATUS, i) = "success";
  }
  setProperty(PropertyNames::OUTPUT_WORKSPACE, outputWS);
}

//----------------------------------------------------------------------------------------------
/** Validate the algorithm's properties.
 *
 * @return A map of porperty names and their issues.
 */
std::map<std::string, std::string> CreateEPP::validateInputs(void) {
  std::map<std::string, std::string> issues;
  API::MatrixWorkspace_sptr inputWS =
      getProperty(PropertyNames::INPUT_WORKSPACE);
  if (!inputWS->run().hasProperty("Ei")) {
    issues[PropertyNames::INPUT_WORKSPACE] =
        "Workspace is missing the 'Ei' sample log.";
  }
  return issues;
}

} // namespace Algorithms
} // namespace Mantid

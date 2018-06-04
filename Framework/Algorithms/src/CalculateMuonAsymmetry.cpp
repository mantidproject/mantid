//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidAlgorithms/CalculateMuonAsymmetry.h"
#include "MantidAlgorithms/MuonAsymmetryHelper.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/StartsWithValidator.h"

#include "MantidKernel/ListValidator.h"
#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/WorkspaceFactory.h"


#include <cmath>
#include <numeric>
#include <vector>

namespace Mantid {
	namespace Algorithms {

		using namespace Kernel;
		using API::Progress;
		using std::size_t;

		// Register the class into the algorithm factory
		DECLARE_ALGORITHM(CalculateMuonAsymmetry)

		/** Initialisation method. Declares properties to be used in algorithm.
		*
		*/
		void CalculateMuonAsymmetry::init() {
			// norm table to update
			declareProperty(
				make_unique<API::WorkspaceProperty<API::ITableWorkspace>>(
					"NormalizationTable", "", Direction::Input),
				"Name of the table containing the normalisations for the asymmetries.");
			// list of uNonrm workspaces to fit to
			declareProperty(Kernel::make_unique<Kernel::ArrayProperty<std::string>>(
				"UnNormalizedWorkspaceList", boost::make_shared<API::ADSValidator>()),
				"An ordered list of workspaces (to get the initial values "
				"for the normalisations).");
			// list of workspaces to output renormalized result to 
			declareProperty(Kernel::make_unique<Kernel::ArrayProperty<std::string>>(
				"ReNormalizedWorkspaceList", boost::make_shared<API::ADSValidator>()),
				"An ordered list of workspaces (to get the initial values "
				"for the normalisations).");

			declareProperty(
				"OutputFitWorkspace", "fit", 
				"The name of the output fit workspace.");

			declareProperty(
				"StartX", 0.1,
				"The lower limit for calculating the asymmetry (an X value).");
			declareProperty(
				"EndX", 15.0,
				"The upper limit for calculating the asymmetry  (an X value).");
			declareProperty(make_unique<API::FunctionProperty>("InputFunction"),
				"The fitting function to be converted.");

			std::vector<std::string> minimizerOptions =
				API::FuncMinimizerFactory::Instance().getKeys();
			Kernel::IValidator_sptr minimizerValidator =
				boost::make_shared<Kernel::StartsWithValidator>(minimizerOptions);
			declareProperty("Minimizer", "Levenberg-MarquardtMD", minimizerValidator,
				"Minimizer to use for fitting.");
			auto mustBePositive = boost::make_shared<Kernel::BoundedValidator<int>>();
			mustBePositive->setLower(0);
			declareProperty(
				"MaxIterations", 500, mustBePositive->clone(),
				"Stop after this number of iterations if a good fit is not found");

		}
		/*
		* Validate the input parameters
		* @returns map with keys corresponding to properties with errors and values
		* containing the error messages.
		*/
		std::map<std::string, std::string> CalculateMuonAsymmetry::validateInputs() {
			// create the map
			std::map<std::string, std::string> validationOutput;
			// check start and end times
			double startX = getProperty("StartX");
			double endX = getProperty("EndX");
			if (startX > endX) {
			validationOutput["StartX"] = "Start time is after the end time.";
			} else if (startX == endX) {
			validationOutput["StartX"] = "Start and end times are equal, there is no "
			"data to apply the algorithm to.";
			}
			// check inputs
			std::vector<std::string> unnormWS = getProperty("UnNormalizedWorkspaceList");
			std::vector<std::string> normWS = getProperty("ReNormalizedWorkspaceList");
			if (normWS.size() != unnormWS.size()) {
				validationOutput["ReNormalizedWorkspaceList"] = "The ReNormalizedWorkspaceList and UnNormalizedWorkspaceList must contain the same number of workspaces.";
			}
			API::IFunction_sptr tmp = getProperty("InputFunction");
			auto function = boost::dynamic_pointer_cast<API::CompositeFunction>(tmp);
			if (function->getNumberDomains() != normWS.size()) {
				validationOutput["InputFunction"] = "The Fitting function does not have the same number of domains as the number of domains to fit.";
			}

			// check norm table is correct -> move this to helper when move muon algs to muon folder
			API::ITableWorkspace_const_sptr tabWS = getProperty("NormalizationTable");

			if (tabWS->columnCount() == 0) {
				validationOutput["NormalizationTable"] =
					"Please provide a non-empty NormalizationTable.";
			}
			// NormalizationTable should have three columns: (norm, name, method)
			if (tabWS->columnCount() != 3) {
				validationOutput["NormalizationTable"] = "NormalizationTable must have three columns";
			}
			auto names = tabWS->getColumnNames();
			int normCount = 0;
			int wsNamesCount = 0;
			for (const std::string &name : names) {

				if (name == "norm") {
					normCount += 1;
				}

				if (name == "name") {
					wsNamesCount += 1;
				}
			}
			if (normCount == 0) {
				validationOutput["NormalizationTable"] = "NormalizationTable needs norm column";
			}
			if (wsNamesCount == 0) {
				validationOutput["NormalizationTable"] = "NormalizationTable needs a name column";
			}
			if (normCount > 1) {
				validationOutput["NormalizationTable"] =
					"NormalizationTable has " + std::to_string(normCount) + " norm columns";
			}
			if (wsNamesCount > 1) {
				validationOutput["NormalizationTable"] =
					"NormalizationTable has " + std::to_string(wsNamesCount) + " name columns";
			}
			
			return validationOutput;
		}
		/** Executes the algorithm
		*
		*/

		void CalculateMuonAsymmetry::exec() {
			const std::vector<std::string> wsNamesUnNorm = getProperty("UnNormalizedWorkspaceList");	
			std::vector<std::string> wsNames = getProperty("reNormalizedWorkspaceList");

			// get new norm
			std::vector<double> norms = getNormConstants(); // this will do the fit
			// update the ws to new norm
			for (size_t j = 0; j < wsNames.size();j++) {
				API::MatrixWorkspace_sptr ws = API::AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>(wsNamesUnNorm[j]);
				API::MatrixWorkspace_sptr normWS = API::AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>(wsNames[j]);

				normWS->mutableY(0) = ws->y(0) / norms[j];
				normWS->mutableY(0) -= 1.0;
				normWS->mutableE(0) = ws->e(0) / norms[j];
			}
			// update table with new norm
			std::vector<std::string> methods(wsNames.size(), "Calculated");
			API::ITableWorkspace_sptr table = getProperty("NormalizationTable");
			updateNormalizationTable(table, wsNames, norms, methods);
}

/**
 * Calculate normalisation constant after the exponential decay has been removed
 * to a linear fitting function
 * @param ws ::  workspace
 * @param wsIndex :: workspace index
 * @param estNormConstant :: estimate of normalisation constant
 * @param startX :: the smallest x value for the fit
 * @param endX :: the largest x value for the fit
 * @return normalisation constant
*/

		std::vector<double> CalculateMuonAsymmetry::getNormConstants() {
			const std::vector<std::string> wsNames = getProperty("UnNormalizedWorkspaceList");
			std::vector<double> norms;

			double startX = getProperty("StartX");
			double endX = getProperty("EndX");
			int maxIterations = getProperty("MaxIterations");
			auto minimizer = getProperty("Minimizer");
			API::IAlgorithm_sptr fit = API::AlgorithmManager::Instance().create("Fit");
			fit->initialize();

			API::IFunction_sptr function = getProperty("InputFunction");
	
			fit->setProperty("Function", function);
			

			fit->setProperty("MaxIterations", maxIterations);

			fit->setPropertyValue("Minimizer", minimizer);

			std::string output = getPropertyValue("OutputFitWorkspace");

			fit->setProperty("Output", output);

			fit->setProperty("InputWorkspace", wsNames[0]);
			fit->setProperty("StartX", startX);
			fit->setProperty("EndX", endX);
			fit->setProperty("WorkspaceIndex", 0);

			if (wsNames.size() > 1) {
				for (size_t j = 1; j < wsNames.size(); j++) {
					std::string suffix = boost::lexical_cast<std::string>(j);

					fit->setPropertyValue("InputWorkspace_" + suffix, wsNames[j]);
					fit->setProperty("WorkspaceIndex_" + suffix, 0);
					fit->setProperty("StartX_" + suffix, startX);
					fit->setProperty("EndX_" + suffix, endX);
				}
			}

			fit->execute();

			std::string fitStatus = fit->getProperty("OutputStatus");

			API::IFunction_sptr tmp = fit->getProperty("Function");
			try {
				if (wsNames.size() == 1) {
					// N(1+g) + exp
					auto result = boost::dynamic_pointer_cast<API::CompositeFunction>(tmp);

					// getFunction(0) -> N(1+g)
					auto TFFunc =
						boost::dynamic_pointer_cast<API::CompositeFunction>(result->getFunction(0));

					// getFunction(0) -> N
					TFFunc =
						boost::dynamic_pointer_cast<API::CompositeFunction>(TFFunc->getFunction(0));
					double norm = TFFunc->getParameter("f0.A0");
					norms.push_back(norm);
				}
				else {
					auto result = boost::dynamic_pointer_cast<API::MultiDomainFunction>(tmp);
					for (size_t j = 0; j < wsNames.size(); j++) {
						// get domain
						auto TFFunc = boost::dynamic_pointer_cast<API::CompositeFunction>(result->getFunction(j));
						// N(1+g) + exp
						TFFunc = boost::dynamic_pointer_cast<API::CompositeFunction>(TFFunc);

						// getFunction(0) -> N(1+g)
						TFFunc =
							boost::dynamic_pointer_cast<API::CompositeFunction>(TFFunc->getFunction(0));

						// getFunction(0) -> N
						TFFunc =
							boost::dynamic_pointer_cast<API::CompositeFunction>(TFFunc->getFunction(0));
						double norm = TFFunc->getParameter("f0.A0");
						norms.push_back(norm);
					}
				}
			}
			catch (...) {
				throw std::invalid_argument("The fitting function is not of the expected form. Try using ConvertFitFunctionForMuonTFAsymmetry");
			}
  return norms;
}

} // namespace Algorithm
} // namespace Mantid

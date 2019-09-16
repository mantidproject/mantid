#include "ALFView_model.h"
// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"


namespace MantidQt {
	namespace CustomInterfaces {
		namespace Direct {
			/*
			 *Loads data for use in ALFView
			 * Loads data, normalise to current and then converts to d spacing
			 * @param name:: string name for ALF data
			 */
			void loadData(const std::string& name) {
				Mantid::API::IAlgorithm_sptr alg =
					Mantid::API::AlgorithmManager::Instance().create("Load");
				alg->initialize();
				alg->setProperty("Filename", name);
				alg->setProperty("OutputWorkspace", "ALF");
				alg->execute();

				// check it is a valid ALF run number
				Mantid::API::MatrixWorkspace_const_sptr ws =
					Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>("ALF");
                                auto da = ws->getNEvents();
                                auto sadfas = ws->size();
                                auto gfgds = ws->readX(0);

				if( ws->getNEvents() != 0){
                                  return;
                                }

  Mantid::API::IAlgorithm_sptr normAlg =
      Mantid::API::AlgorithmManager::Instance().create("NormaliseByCurrent");
  normAlg->initialize();
  normAlg->setProperty("InputWorkspace", "ALF");
  normAlg->setProperty("OutputWorkspace", "ALF");
  normAlg->execute();

  Mantid::API::IAlgorithm_sptr dSpacingAlg =
      Mantid::API::AlgorithmManager::Instance().create("ConvertUnits");
  dSpacingAlg->initialize();
  dSpacingAlg->setProperty("InputWorkspace", "ALF");
  dSpacingAlg->setProperty("Target", "dSpacing");
  dSpacingAlg->setProperty("OutputWorkspace", "ALF");
  dSpacingAlg->execute();
}

} // namespace Direct
} // namespace CustomInterfaces
} // namespace MantidQt
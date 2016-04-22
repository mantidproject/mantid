#ifndef MANTID_CUSTOMINTERFACES_REFLGENERICDATAPROCESSORPRESENTERFACTORY_H
#define MANTID_CUSTOMINTERFACES_REFLGENERICDATAPROCESSORPRESENTERFACTORY_H

#include "GenericDataProcessorPresenter.h"
#include "GenericDataProcessorPresenterFactory.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataPostprocessorAlgorithm.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataPreprocessorAlgorithm.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorAlgorithm.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorWhiteList.h"

namespace MantidQt {
namespace CustomInterfaces {
/** @class ReflGenericDataProcessorPresenterFactory

ReflGenericDataProcessorPresenterFactory creates a Reflectometry
GenericDataProcessorPresenter

Copyright &copy; 2011-14 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class ReflGenericDataProcessorPresenterFactory
    : public GenericDataProcessorPresenterFactory {
public:
  ReflGenericDataProcessorPresenterFactory() = default;
  virtual ~ReflGenericDataProcessorPresenterFactory() = default;

  /**
  * Creates a Reflectometry Data Processor Presenter
  */
  boost::shared_ptr<GenericDataProcessorPresenter> create() override {

    // The whitelist, elements will appear in order in the table
    // 'Run(s)' column will be linked to 'InputWorkspace' property
    // 'Angle' column will be linked to 'ThetaIn'
    // 'Transmission Run(s)' column will be linked to 'FirstTransmissionRun'
    // 'Q min' column will be linked to 'MomentumTransferMinimum'
    // 'Q max' column will be linked to 'MomentumTransferMaximum'
    // 'dq/Q' column will be linked to 'MomentumTransferStep'
    // 'Scale' column will be linked to 'ScaleFactor'
    DataProcessorWhiteList whitelist;
    whitelist.addElement("Run(s)", "InputWorkspace");
    whitelist.addElement("Angle", "ThetaIn");
    whitelist.addElement("Transmission Run(s)", "FirstTransmissionRun");
    whitelist.addElement("Q min", "MomentumTransferMinimum");
    whitelist.addElement("Q max", "MomentumTransferMaximum");
    whitelist.addElement("dQ/Q", "MomentumTransferStep");
    whitelist.addElement("Scale", "ScaleFactor");

    // The data processor algorithm
    DataProcessorAlgorithm processor(
        /*The name of the algorithm */
        "ReflectometryReductionOneAuto",
        /*Prefixes to the output workspaces*/
        std::vector<std::string>{"IvsQ_", "IvsLam_"},
        /*The blacklist*/
        std::set<std::string>{"ThetaIn", "ThetaOut", "InputWorkspace",
                              "OutputWorkspace", "OutputWorkspaceWavelength",
                              "FirstTransmissionRun", "SecondTransmissionRun"});

    // Pre-processing instructions as a map:
    // Keys are the column names
    // Values are the associated pre-processing algorithms
    std::map<std::string, DataPreprocessorAlgorithm> preprocessMap = {
        /*This pre-processor will be applied to column 'Run(s)'*/
        {/*The name of the column*/ "Run(s)",
         /*The pre-processor algorithm, 'Plus' by default*/ DataPreprocessorAlgorithm()},
        /*This pre-processor will be applied to column 'Transmission Run(s)'*/
        {/*The name of the column*/ "Transmission Run(s)",
         /*The pre-processor algorithm: CreateTransmissionWorkspaceAuto*/
         DataPreprocessorAlgorithm(
             "CreateTransmissionWorkspaceAuto",
             /*Prefix for the output workspace*/
             std::vector<std::string>{"TRANS_"},
             /*Blacklist of properties we don't want to show*/
             std::set<std::string>{"FirstTransmissionRun",
                                   "SecondTransmissionRun",
                                   "OutputWorkspace"})}};

    // The post-processor algorithm's name, 'Stitch1DMany' by default
    DataPostprocessorAlgorithm postprocessor;

    return boost::make_shared<GenericDataProcessorPresenter>(
        whitelist, preprocessMap, processor, postprocessor);
  };
};
}
}
#endif /*MANTID_CUSTOMINTERFACES_REFLGENERICDATAPROCESSORPRESENTERFACTORY_H*/
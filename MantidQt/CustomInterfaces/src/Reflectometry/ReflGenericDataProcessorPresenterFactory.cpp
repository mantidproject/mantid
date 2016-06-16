#include "MantidQtCustomInterfaces/Reflectometry/ReflGenericDataProcessorPresenterFactory.h"

namespace MantidQt {
namespace CustomInterfaces {

using namespace MantidQt::MantidWidgets;

/**
* Creates a Reflectometry Data Processor Presenter
*/
boost::shared_ptr<GenericDataProcessorPresenter>
ReflGenericDataProcessorPresenterFactory::create() {

  // The whitelist, elements will appear in order in the table
  // 'Run(s)' column will be linked to 'InputWorkspace' property
  // 'Angle' column will be linked to 'ThetaIn'
  // 'Transmission Run(s)' column will be linked to 'FirstTransmissionRun'
  // 'Q min' column will be linked to 'MomentumTransferMinimum'
  // 'Q max' column will be linked to 'MomentumTransferMaximum'
  // 'dq/Q' column will be linked to 'MomentumTransferStep'
  // 'Scale' column will be linked to 'ScaleFactor'
  // Descriptions can also be added
  DataProcessorWhiteList whitelist;
  whitelist.addElement("Run(s)", "InputWorkspace",
                       "<b>Sample runs to be processed.</b><br "
                       "/><i>required</i><br />Runs may be given as run "
                       "numbers or workspace names. Multiple runs may be "
                       "added together by separating them with a '+'. <br "
                       "/><br /><b>Example:</b> <samp>1234+1235+1236</samp>");
  whitelist.addElement(
      "Angle", "ThetaIn",
      "<b>Angle used during the run.< / b><br / ><i>optional</i><br />Unit: "
      "degrees<br />If left blank, this is set to the last value for 'THETA' "
      "in the run's sample log. If multiple runs were given in the Run(s) "
      "column, the first listed run's sample log will be used. <br /><br "
      "/><b>Example:</b> <samp>0.7</samp>");
  whitelist.addElement(
      "Transmission Run(s)", "FirstTransmissionRun",
      "<b>Transmission run(s) to use to normalise the sample runs.</b><br "
      "/><i>optional</i><br />To specify two transmission runs, separate "
      "them with a '+'. If left blank, the sample runs will be normalised "
      "by monitor only.<br /><br /><b>Example:</b> <samp>1234+12345</samp>");
  whitelist.addElement("Q min", "MomentumTransferMinimum",
                       "<b>Minimum value of Q to be used</b><br "
                       "/><i>optional</i><br />Unit: &#197;<sup>-1</sup><br "
                       "/>Data with a value of Q lower than this will be "
                       "discarded. If left blank, this is set to the lowest "
                       "Q value found. This is useful for discarding noisy "
                       "data. <br /><br /><b>Example:</b> <samp>0.1</samp>");
  whitelist.addElement("Q max", "MomentumTransferMaximum",
                       "<b>Maximum value of Q to be used</b><br "
                       "/><i>optional</i><br />Unit: &#197;<sup>-1</sup><br "
                       "/>Data with a value of Q higher than this will be "
                       "discarded. If left blank, this is set to the highest "
                       "Q value found. This is useful for discarding noisy "
                       "data. <br /><br /><b>Example:</b> <samp>0.9</samp>");
  whitelist.addElement(
      "dQ/Q", "MomentumTransferStep",
      "<b>Resolution used when rebinning</b><br /><i>optional</i><br />If "
      "left blank, this is calculated for you using the CalculateResolution "
      "algorithm. <br /><br /><b>Example:</b> <samp>0.9</samp>");
  whitelist.addElement(
      "Scale", "ScaleFactor",
      "<b>Scaling factor</b><br /><i>required</i><br />The created IvsQ "
      "workspaces will be Scaled by <samp>1/i</samp> where <samp>i</samp> is "
      "the value of this column. <br /><br /><b>Example:</b> <samp>1</samp>");

  // The data processor algorithm
  DataProcessorProcessingAlgorithm processor(
      /*The name of the algorithm */
      "ReflectometryReductionOneAuto",
      /*Prefixes to the output workspaces*/
      std::vector<std::string>{"IvsQ_", "IvsLam_"},
      /*The blacklist*/
      std::set<std::string>{
          "ThetaIn", "ThetaOut", "InputWorkspace", "OutputWorkspace",
          "OutputWorkspaceWavelength", "FirstTransmissionRun",
          "SecondTransmissionRun", "MomentumTransferMinimum",
          "MomentumTransferMaximum", "MomentumTransferStep", "ScaleFactor"});

  // Pre-processing instructions as a map:
  // Keys are the column names
  // Values are the associated pre-processing algorithms
  std::map<std::string, DataProcessorPreprocessingAlgorithm> preprocessMap = {
      /*This pre-processor will be applied to column 'Run(s)'*/
      {/*The name of the column*/ "Run(s)",
       /*The pre-processor algorithm, 'Plus' by default*/ DataProcessorPreprocessingAlgorithm()},
      /*This pre-processor will be applied to column 'Transmission Run(s)'*/
      {/*The name of the column*/ "Transmission Run(s)",
       /*The pre-processor algorithm: CreateTransmissionWorkspaceAuto*/
       DataProcessorPreprocessingAlgorithm(
           "CreateTransmissionWorkspaceAuto",
           /*Prefix for the output workspace*/
           "TRANS_",
           /*Blacklist of properties we don't want to show*/
           std::set<std::string>{"FirstTransmissionRun",
                                 "SecondTransmissionRun", "OutputWorkspace"},
           /*I don't want to show the transmission runs in the output ws
              name*/
           false)}};

  // The post-processor algorithm's name, 'Stitch1DMany' by default
  DataProcessorPostprocessingAlgorithm postprocessor;

  return boost::make_shared<GenericDataProcessorPresenter>(
      whitelist, preprocessMap, processor, postprocessor);
}
}
}

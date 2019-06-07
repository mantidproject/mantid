// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ReflGenericDataProcessorPresenterFactory.h"

namespace MantidQt {
namespace CustomInterfaces {

using namespace MantidQt::MantidWidgets;

/**
 * Creates a Reflectometry Data Processor Presenter
 */
std::unique_ptr<ReflDataProcessorPresenter>
ReflGenericDataProcessorPresenterFactory::create(int group) {

  // The whitelist, elements will appear in order in the table
  // 'Run(s)' column will be linked to 'InputWorkspace' property
  // 'Angle' column will be linked to 'ThetaIn'
  // 'Transmission Run(s)' column will be linked to 'FirstTransmissionRun'
  // 'Q min' column will be linked to 'MomentumTransferMinimum'
  // 'Q max' column will be linked to 'MomentumTransferMaximum'
  // 'dq/Q' column will be linked to 'MomentumTransferStep'
  // 'Scale' column will be linked to 'ScaleFactor'
  // Descriptions can also be added
  WhiteList whitelist;
  whitelist.addElement("Run(s)", "InputWorkspace",
                       "<b>Sample runs to be processed.</b><br "
                       "/><i>required</i><br />Runs may be given as run "
                       "numbers or workspace names. Multiple runs may be "
                       "added together by separating them with a '+'. <br "
                       "/><br /><b>Example:</b> <samp>1234+1235+1236</samp>",
                       true);
  whitelist.addElement(
      "Angle", "ThetaIn",
      "<b>Angle used during the run.< / b><br / ><i>optional</i><br />Unit: "
      "degrees<br />If left blank, this is set to the last value for 'THETA' "
      "in the run's sample log. If multiple runs were given in the Run(s) "
      "column, the first listed run's sample log will be used. <br /><br "
      "/><b>Example:</b> <samp>0.7</samp>",
      false, "", true);
  whitelist.addElement(
      "Transmission Run(s)", "FirstTransmissionRun",
      "<b>Transmission run(s) to use to normalise the sample runs.</b><br "
      "/><i>optional</i><br />To specify two transmission runs, separate "
      "them with a '+'. If left blank, the sample runs will be normalised "
      "by monitor only.<br /><br /><b>Example:</b> <samp>1234+12345</samp>");
  whitelist.addElement("Q min", "MomentumTransferMin",
                       "<b>Minimum value of Q to be used</b><br "
                       "/><i>optional</i><br />Unit: &#197;<sup>-1</sup><br "
                       "/>Data with a value of Q lower than this will be "
                       "discarded. If left blank, this is set to the lowest "
                       "Q value found. This is useful for discarding noisy "
                       "data. <br /><br /><b>Example:</b> <samp>0.1</samp>");
  whitelist.addElement("Q max", "MomentumTransferMax",
                       "<b>Maximum value of Q to be used</b><br "
                       "/><i>optional</i><br />Unit: &#197;<sup>-1</sup><br "
                       "/>Data with a value of Q higher than this will be "
                       "discarded. If left blank, this is set to the highest "
                       "Q value found. This is useful for discarding noisy "
                       "data. <br /><br /><b>Example:</b> <samp>0.9</samp>");
  whitelist.addElement(
      "dQ/Q", "MomentumTransferStep",
      "<b>Resolution used when rebinning</b><br /><i>optional</i><br />If "
      "left blank, this is calculated for you using the "
      "NRCalculateSlitResolution "
      "algorithm. <br /><br /><b>Example:</b> <samp>0.9</samp>");
  whitelist.addElement(
      "Scale", "ScaleFactor",
      "<b>Scaling factor</b><br /><i>required</i><br />The created IvsQ "
      "workspaces will be Scaled by <samp>1/i</samp> where <samp>i</samp> is "
      "the value of this column. <br /><br /><b>Example:</b> <samp>1</samp>");

  // The data processor algorithm
  ProcessingAlgorithm processor(
      /*The name of the algorithm */
      "ReflectometryReductionOneAuto",
      /*Prefixes to the output workspaces*/
      std::vector<QString>{"IvsQ_binned_", "IvsQ_", "IvsLam_"}, 1,
      /*The blacklist*/
      std::set<QString>{"ThetaIn", "ThetaOut", "InputWorkspace",
                        "OutputWorkspace", "OutputWorkspaceBinned",
                        "OutputWorkspaceWavelength", "FirstTransmissionRun",
                        "SecondTransmissionRun", "MomentumTransferMin",
                        "MomentumTransferMax", "MomentumTransferStep",
                        "ScaleFactor"});

  // Pre-processing instructions as a map:
  // Keys are the column names
  // Values are the pre-processing algorithms that will be applied to columns
  std::map<QString, PreprocessingAlgorithm> preprocessMap = {
      /* 'Plus' will be applied to column 'Run(s)'*/
      {"Run(s)",
       PreprocessingAlgorithm("Plus", "TOF_", "+",
                              std::set<QString>{"LHSWorkspace", "RHSWorkspace",
                                                "OutputWorkspace"})},
      /* 'CreateTransmissionWorkspaceAuto' will be applied to column
         'Transmission Run(s)'*/
      {"Transmission Run(s)",
       PreprocessingAlgorithm("CreateTransmissionWorkspaceAuto", "TRANS_", "_",
                              std::set<QString>{"FirstTransmissionRun",
                                                "SecondTransmissionRun",
                                                "OutputWorkspace"})}};

  // The post-processing algorithm
  PostprocessingAlgorithm postprocessor(
      "Stitch1DMany", "IvsQ_",
      std::set<QString>{"InputWorkspaces", "OutputWorkspace"});

  // Post-processing instructions linking column names to properties of the
  // post-processing algorithm
  // Key is column name
  // Value is property name of the post-processing algorithm
  std::map<QString, QString> postprocessMap = {{"dQ/Q", "Params"}};

  return std::make_unique<ReflDataProcessorPresenter>(
      whitelist, preprocessMap, processor, postprocessor, group, postprocessMap,
      "LoadNexus");
}
} // namespace CustomInterfaces
} // namespace MantidQt

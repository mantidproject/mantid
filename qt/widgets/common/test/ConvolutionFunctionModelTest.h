// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidQtWidgets/Common/ConvolutionFunctionModel.h"
#include <cxxtest/TestSuite.h>
#include <iostream>

using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;

class ConvolutionFunctionModelTest : public CxxTest::TestSuite {

public:
  void setUp() override {
    // Needs other algorithms and functions to be registered
    FrameworkManager::Instance();
  }

  void test_empty() {
    ConvolutionFunctionModel model;
    TS_ASSERT(!model.getFitFunction());
  }

  void test_clear() {
    ConvolutionFunctionModel model;
    model.clear();
  }

  void test_no_convolution() {
    ConvolutionFunctionModel model;
    TS_ASSERT_THROWS_EQUALS(model.setFunctionString("name=LinearBackground,A0=1,A1=2"), const std::runtime_error &e,
                            std::string(e.what()), "Model doesn't contain a convolution.");
  }

  void test_no_convolution_2() {
    ConvolutionFunctionModel model;
    TS_ASSERT_THROWS_EQUALS(model.setFunctionString("name=LinearBackground;name=Lorentzian"),
                            const std::runtime_error &e, std::string(e.what()), "Model doesn't contain a convolution.");
  }

  void test_two_backgrounds() {
    ConvolutionFunctionModel model;
    TS_ASSERT_THROWS_EQUALS(model.setFunctionString("name=LinearBackground;name=FlatBackground;composite=Convolution"),
                            const std::runtime_error &e, std::string(e.what()),
                            "Model cannot have more than one background.");
  }

  void test_wrong_resolution() {
    ConvolutionFunctionModel model;
    TS_ASSERT_THROWS_EQUALS(model.setFunctionString("composite=Convolution;name=Gaussian;name=Lorentzian"),
                            const std::runtime_error &e, std::string(e.what()),
                            "Model's resolution function must have type Resolution.");
  }

  void test_empty_convolution() {
    ConvolutionFunctionModel model;
    TS_ASSERT_THROWS_NOTHING(model.setFunctionString("composite=Convolution"));
    TS_ASSERT(!model.backgroundPrefix());
    TS_ASSERT_EQUALS(*model.convolutionPrefix(), "");
    TS_ASSERT_EQUALS(model.resolutionWorkspace(), "");
    TS_ASSERT_EQUALS(model.resolutionWorkspaceIndex(), 0);
    TS_ASSERT(!model.deltaFunctionPrefix());
    TS_ASSERT(!model.peakPrefixes());
  }

  void test_background_empty_convolution() {
    ConvolutionFunctionModel model;
    TS_ASSERT_THROWS_NOTHING(model.setFunctionString("name=LinearBackground;composite=Convolution"));
    TS_ASSERT_EQUALS(*model.backgroundPrefix(), "f0.");
    TS_ASSERT_EQUALS(*model.convolutionPrefix(), "f1.");
    TS_ASSERT_EQUALS(model.resolutionWorkspace(), "");
    TS_ASSERT_EQUALS(model.resolutionWorkspaceIndex(), 0);
    TS_ASSERT(!model.deltaFunctionPrefix());
    TS_ASSERT(!model.peakPrefixes());
  }

  void test_background_before_convolution() {
    ConvolutionFunctionModel model;
    TS_ASSERT_THROWS_NOTHING(model.setFunctionString("name=LinearBackground;(composite=Convolution;"
                                                     "name=Resolution;name=Lorentzian)"));
    TS_ASSERT_EQUALS(model.backgroundPrefix()->toStdString(), "f0.");
    TS_ASSERT_EQUALS(model.convolutionPrefix()->toStdString(), "f1.");
    TS_ASSERT_EQUALS(model.resolutionWorkspace(), "");
    TS_ASSERT_EQUALS(model.resolutionWorkspaceIndex(), 0);
    TS_ASSERT(!model.deltaFunctionPrefix());
    TS_ASSERT(model.peakPrefixes());
    TS_ASSERT_EQUALS(model.peakPrefixes()->at(0).toStdString(), "f1.f1.");
  }

  void test_background_after_convolution() {
    ConvolutionFunctionModel model;
    TS_ASSERT_THROWS_NOTHING(model.setFunctionString("(composite=Convolution;name=Resolution;name="
                                                     "Lorentzian);name=LinearBackground"));
    TS_ASSERT_EQUALS(model.backgroundPrefix()->toStdString(), "f1.");
    TS_ASSERT_EQUALS(model.convolutionPrefix()->toStdString(), "f0.");
    TS_ASSERT_EQUALS(model.resolutionWorkspace(), "");
    TS_ASSERT_EQUALS(model.resolutionWorkspaceIndex(), 0);
    TS_ASSERT(!model.deltaFunctionPrefix());
    TS_ASSERT(model.peakPrefixes());
    TS_ASSERT_EQUALS(model.peakPrefixes()->at(0).toStdString(), "f0.f1.");
  }

  void test_two_peaks() {
    ConvolutionFunctionModel model;
    TS_ASSERT_THROWS_NOTHING(model.setFunctionString("(composite=Convolution;name=Resolution;name="
                                                     "Lorentzian;name=Lorentzian);"
                                                     "name=LinearBackground"));
    TS_ASSERT_EQUALS(model.backgroundPrefix()->toStdString(), "f1.");
    TS_ASSERT_EQUALS(model.convolutionPrefix()->toStdString(), "f0.");
    TS_ASSERT_EQUALS(model.resolutionWorkspace(), "");
    TS_ASSERT_EQUALS(model.resolutionWorkspaceIndex(), 0);
    TS_ASSERT(!model.deltaFunctionPrefix());
    TS_ASSERT(model.peakPrefixes());
    TS_ASSERT_EQUALS(model.peakPrefixes()->at(0).toStdString(), "f0.f1.f0.");
    TS_ASSERT_EQUALS(model.peakPrefixes()->at(1).toStdString(), "f0.f1.f1.");
  }

  void test_two_peaks_no_background() {
    ConvolutionFunctionModel model;
    TS_ASSERT_THROWS_NOTHING(model.setFunctionString("composite=Convolution;name=Resolution;name="
                                                     "Lorentzian;name=Lorentzian"));
    TS_ASSERT(!model.backgroundPrefix());
    TS_ASSERT_EQUALS(model.convolutionPrefix()->toStdString(), "");
    TS_ASSERT_EQUALS(model.resolutionWorkspace(), "");
    TS_ASSERT_EQUALS(model.resolutionWorkspaceIndex(), 0);
    TS_ASSERT(!model.deltaFunctionPrefix());
    TS_ASSERT(model.peakPrefixes());
    TS_ASSERT_EQUALS(model.peakPrefixes()->at(0).toStdString(), "f1.f0.");
    TS_ASSERT_EQUALS(model.peakPrefixes()->at(1).toStdString(), "f1.f1.");
  }

  void test_delta() {
    ConvolutionFunctionModel model;
    TS_ASSERT_THROWS_NOTHING(model.setFunctionString("(composite=Convolution;name=Resolution;name="
                                                     "DeltaFunction);name=LinearBackground"));
    TS_ASSERT_EQUALS(model.backgroundPrefix()->toStdString(), "f1.");
    TS_ASSERT_EQUALS(model.convolutionPrefix()->toStdString(), "f0.");
    TS_ASSERT_EQUALS(model.resolutionWorkspace(), "");
    TS_ASSERT_EQUALS(model.resolutionWorkspaceIndex(), 0);
    TS_ASSERT_EQUALS(model.deltaFunctionPrefix()->toStdString(), "f0.f1.");
    TS_ASSERT(!model.peakPrefixes());
  }

  void test_delta_no_background() {
    ConvolutionFunctionModel model;
    TS_ASSERT_THROWS_NOTHING(model.setFunctionString("composite=Convolution;name=Resolution;name=DeltaFunction"));
    TS_ASSERT(!model.backgroundPrefix());
    TS_ASSERT_EQUALS(model.convolutionPrefix()->toStdString(), "");
    TS_ASSERT_EQUALS(model.resolutionWorkspace(), "");
    TS_ASSERT_EQUALS(model.resolutionWorkspaceIndex(), 0);
    TS_ASSERT_EQUALS(model.deltaFunctionPrefix()->toStdString(), "f1.");
    TS_ASSERT(!model.peakPrefixes());
  }

  void test_two_peaks_no_background_delta() {
    ConvolutionFunctionModel model;
    TS_ASSERT_THROWS_NOTHING(model.setFunctionString("composite=Convolution;name=Resolution;name="
                                                     "Lorentzian;name=Lorentzian;name=DeltaFunction"));
    TS_ASSERT(!model.backgroundPrefix());
    TS_ASSERT_EQUALS(model.convolutionPrefix()->toStdString(), "");
    TS_ASSERT_EQUALS(model.resolutionWorkspace(), "");
    TS_ASSERT_EQUALS(model.resolutionWorkspaceIndex(), 0);
    TS_ASSERT_EQUALS(model.deltaFunctionPrefix()->toStdString(), "f1.f2.");
    TS_ASSERT(model.peakPrefixes());
    TS_ASSERT_EQUALS(model.peakPrefixes()->at(0).toStdString(), "f1.f0.");
    TS_ASSERT_EQUALS(model.peakPrefixes()->at(1).toStdString(), "f1.f1.");
  }

  void test_two_peaks_delta() {
    ConvolutionFunctionModel model;
    TS_ASSERT_THROWS_NOTHING(model.setFunctionString("(composite=Convolution;name=Resolution;name="
                                                     "DeltaFunction;name=Lorentzian;name=Lorentzian);"
                                                     "name=LinearBackground"));
    TS_ASSERT_EQUALS(model.backgroundPrefix()->toStdString(), "f1.");
    TS_ASSERT_EQUALS(model.convolutionPrefix()->toStdString(), "f0.");
    TS_ASSERT_EQUALS(model.resolutionWorkspace(), "");
    TS_ASSERT_EQUALS(model.resolutionWorkspaceIndex(), 0);
    TS_ASSERT_EQUALS(model.deltaFunctionPrefix()->toStdString(), "f0.f1.f0.");
    TS_ASSERT(model.peakPrefixes());
    TS_ASSERT_EQUALS(model.peakPrefixes()->at(0).toStdString(), "f0.f1.f1.");
    TS_ASSERT_EQUALS(model.peakPrefixes()->at(1).toStdString(), "f0.f1.f2.");
  }

  void test_resolution_workspace() {
    auto algo = AlgorithmManager::Instance().create("CreateWorkspace");
    algo->initialize();
    algo->setPropertyValue("DataX", "1,2,3");
    algo->setPropertyValue("DataY", "1,2,3");
    algo->setPropertyValue("OutputWorkspace", "abc");
    algo->execute();
    ConvolutionFunctionModel model;
    TS_ASSERT_THROWS_NOTHING(model.setFunctionString("composite=Convolution;name=Resolution,"
                                                     "Workspace=\"abc\";name=Lorentzian"));
    TS_ASSERT(!model.backgroundPrefix());
    TS_ASSERT_EQUALS(model.convolutionPrefix()->toStdString(), "");
    TS_ASSERT_EQUALS(model.resolutionWorkspace(), "abc");
    TS_ASSERT_EQUALS(model.resolutionWorkspaceIndex(), 0);
    TS_ASSERT(!model.deltaFunctionPrefix());
    TS_ASSERT(model.peakPrefixes());
    TS_ASSERT_EQUALS(model.peakPrefixes()->at(0).toStdString(), "f1.");
  }

  void test_Lorentzian_can_be_combined_with_additional_fit_type() {
    ConvolutionFunctionModel model;
    TS_ASSERT_THROWS_NOTHING(model.setFunctionString("(composite=Convolution;name=Resolution;"
                                                     "name=Lorentzian;name=Lorentzian;name=TeixeiraWaterSQE);"
                                                     "name=LinearBackground"));

    TS_ASSERT_EQUALS(model.backgroundPrefix()->toStdString(), "f1.");
    TS_ASSERT_EQUALS(model.convolutionPrefix()->toStdString(), "f0.");
    TS_ASSERT(model.peakPrefixes());
    TS_ASSERT_EQUALS(model.peakPrefixes()->at(0).toStdString(), "f0.f1.f0.");
    TS_ASSERT_EQUALS(model.peakPrefixes()->at(1).toStdString(), "f0.f1.f1.");
    TS_ASSERT_EQUALS(model.fitTypePrefix()->toStdString(), "f0.f1.f2.");
  }

  void test_Lorentzian_can_be_combined_with_additional_fit_type_and_delta() {
    ConvolutionFunctionModel model;
    TS_ASSERT_THROWS_NOTHING(model.setFunctionString("(composite=Convolution;name=Resolution;name="
                                                     "DeltaFunction;name=Lorentzian;name=Lorentzian;"
                                                     "name=TeixeiraWaterSQE);"
                                                     "name=LinearBackground"));

    TS_ASSERT_EQUALS(model.backgroundPrefix()->toStdString(), "f1.");
    TS_ASSERT_EQUALS(model.convolutionPrefix()->toStdString(), "f0.");
    TS_ASSERT(model.peakPrefixes());
    TS_ASSERT_EQUALS(model.deltaFunctionPrefix()->toStdString(), "f0.f1.f0.");
    TS_ASSERT_EQUALS(model.peakPrefixes()->at(0).toStdString(), "f0.f1.f1.");
    TS_ASSERT_EQUALS(model.peakPrefixes()->at(1).toStdString(), "f0.f1.f2.");
    TS_ASSERT_EQUALS(model.fitTypePrefix()->toStdString(), "f0.f1.f3.");
  }
  void test_Lorentzian_can_be_combined_with_additional_fit_type_and_temp_correction() {
    auto algo = AlgorithmManager::Instance().create("CreateWorkspace");
    algo->initialize();
    algo->setPropertyValue("DataX", "1,2,3");
    algo->setPropertyValue("DataY", "1,2,3");
    algo->setPropertyValue("OutputWorkspace", "abc");
    algo->execute();
    ConvolutionFunctionModel model;
    model.setNumberDomains(2);
    auto pair1 = std::make_pair<std::string, int>("abc", 1);
    auto pair2 = std::make_pair<std::string, int>("abc", 2);
    auto fitResolutions = std::vector<std::pair<std::string, size_t>>();
    fitResolutions.emplace_back(pair1);
    fitResolutions.emplace_back(pair2);

    model.setModel("name=FlatBackground", fitResolutions, "(name=Lorentzian;name=Lorentzian)",
                   "(name=TeixeiraWaterSQE)", false, std::vector<double>(), false, true, 100.0);

    TS_ASSERT(model.peakPrefixes());
    TS_ASSERT_EQUALS(model.backgroundPrefix().value().toStdString(), "f0.");
    TS_ASSERT_EQUALS(model.convolutionPrefix().value().toStdString(), "f1.");
    TS_ASSERT_EQUALS(model.peakPrefixes().value()[0].toStdString(), "f1.f1.f1.f0.");
    TS_ASSERT_EQUALS(model.peakPrefixes().value()[1].toStdString(), "f1.f1.f1.f1.");
    TS_ASSERT_EQUALS(model.fitTypePrefix()->toStdString(), "f1.f1.f1.f2.");
    TS_ASSERT_EQUALS(model.tempFunctionPrefix().value().toStdString(), "f1.f1.f0.");
  }

  void test_resolution_workspace_index() {
    auto algo = AlgorithmManager::Instance().create("CreateWorkspace");
    algo->initialize();
    algo->setPropertyValue("DataX", "1,2,3");
    algo->setPropertyValue("DataY", "1,2,3");
    algo->setPropertyValue("OutputWorkspace", "abc");
    algo->execute();
    ConvolutionFunctionModel model;
    TS_ASSERT_THROWS_NOTHING(model.setFunctionString("composite=Convolution;name=Resolution,Workspace=\"abc\","
                                                     "WorkspaceIndex=3;name=Lorentzian"));
    TS_ASSERT(!model.backgroundPrefix());
    TS_ASSERT_EQUALS(model.convolutionPrefix()->toStdString(), "");
    TS_ASSERT_EQUALS(model.resolutionWorkspace(), "abc");
    TS_ASSERT_EQUALS(model.resolutionWorkspaceIndex(), 3);
    TS_ASSERT(!model.deltaFunctionPrefix());
    TS_ASSERT(model.peakPrefixes());
    TS_ASSERT_EQUALS(model.peakPrefixes()->at(0).toStdString(), "f1.");
  }

  void test_setModel_with_resolution_workspace_list_creates_correct_function() {
    auto algo = AlgorithmManager::Instance().create("CreateWorkspace");
    algo->initialize();
    algo->setPropertyValue("DataX", "1,2,3");
    algo->setPropertyValue("DataY", "1,2,3");
    algo->setPropertyValue("OutputWorkspace", "abc");
    algo->execute();
    ConvolutionFunctionModel model;
    model.setNumberDomains(2);
    auto pair1 = std::make_pair<std::string, int>("abc", 1);
    auto pair2 = std::make_pair<std::string, int>("abc", 2);
    auto fitResolutions = std::vector<std::pair<std::string, size_t>>();
    fitResolutions.emplace_back(pair1);
    fitResolutions.emplace_back(pair2);

    model.setModel("", fitResolutions, "", "", false, std::vector<double>(), false, false, 100.0);

    auto fitFunctionAsString = model.getFitFunction()->asString();
    TS_ASSERT_EQUALS(fitFunctionAsString, "composite=MultiDomainFunction,NumDeriv=true;(composite=Convolution,"
                                          "NumDeriv=true,FixResolution=true,$domains=i;name=Resolution,Workspace="
                                          "abc,WorkspaceIndex=1,X=(),Y=());(composite=Convolution,NumDeriv="
                                          "true,FixResolution=true,$domains=i;name=Resolution,Workspace=abc,"
                                          "WorkspaceIndex=2,X=(),Y=())");
  }

  void test_setModel_with_delta_function_correct() {
    auto algo = AlgorithmManager::Instance().create("CreateWorkspace");
    algo->initialize();
    algo->setPropertyValue("DataX", "1,2,3");
    algo->setPropertyValue("DataY", "1,2,3");
    algo->setPropertyValue("OutputWorkspace", "abc");
    algo->execute();
    ConvolutionFunctionModel model;
    model.setNumberDomains(2);
    auto pair1 = std::make_pair<std::string, int>("abc", 1);
    auto pair2 = std::make_pair<std::string, int>("abc", 2);
    auto fitResolutions = std::vector<std::pair<std::string, size_t>>();
    fitResolutions.emplace_back(pair1);
    fitResolutions.emplace_back(pair2);

    model.setModel("", fitResolutions, "", "", true, std::vector<double>(), false, false, 100.0);

    auto fitFunctionAsString = model.getFitFunction()->asString();
    TS_ASSERT_EQUALS(fitFunctionAsString, "composite=MultiDomainFunction,NumDeriv=true;(composite=Convolution,"
                                          "NumDeriv=true,FixResolution=true,$domains=i;name=Resolution,Workspace="
                                          "abc,WorkspaceIndex=1,X=(),Y=();name=DeltaFunction,Height=1,Centre=0,"
                                          "constraints=(0<Height));("
                                          "composite=Convolution,NumDeriv="
                                          "true,FixResolution=true,$domains=i;name=Resolution,Workspace=abc,"
                                          "WorkspaceIndex=2,X=(),Y=();name=DeltaFunction,Height=1,Centre=0,"
                                          "constraints=(0<Height))");
  }

  void test_setModel_with_delta_function_TeixeiraWaterSQE_correct() {
    auto algo = AlgorithmManager::Instance().create("CreateWorkspace");
    algo->setPropertyValue("DataX", "1,2,3");
    algo->setPropertyValue("DataY", "1,2,3");
    algo->setPropertyValue("OutputWorkspace", "abc");
    algo->execute();
    ConvolutionFunctionModel model;
    model.setNumberDomains(2);
    auto pair1 = std::make_pair<std::string, int>("abc", 1);
    auto pair2 = std::make_pair<std::string, int>("abc", 2);
    auto fitResolutions = std::vector<std::pair<std::string, size_t>>();
    fitResolutions.emplace_back(pair1);
    fitResolutions.emplace_back(pair2);

    model.setModel("name=FlatBackground", fitResolutions, "", "name=TeixeiraWaterSQE", true, std::vector<double>(),
                   false, false, 100.0);

    auto fitFunctionAsString = model.getFitFunction()->asString();
    TS_ASSERT_EQUALS(fitFunctionAsString, "composite=MultiDomainFunction,NumDeriv=true;(composite="
                                          "CompositeFunction,NumDeriv=false,$domains=i;name=FlatBackground,A0=0;("
                                          "composite=Convolution,NumDeriv=true,FixResolution=true;name="
                                          "Resolution,Workspace=abc,WorkspaceIndex=1,X=(),Y=();(name="
                                          "TeixeiraWaterSQE,Q=8.9884656743115785e+307,WorkspaceIndex=2147483647,"
                                          "Height=1,DiffCoeff=2.3,Tau=1.25,Centre=0;name=DeltaFunction,Height=1,"
                                          "Centre=0,constraints=(0<Height))));(composite=CompositeFunction,"
                                          "NumDeriv=false,$domains=i;"
                                          "name=FlatBackground,A0=0;(composite=Convolution,NumDeriv=true,"
                                          "FixResolution=true;name=Resolution,Workspace=abc,WorkspaceIndex=2,X=()"
                                          ",Y=()"
                                          ";(name=TeixeiraWaterSQE,Q=8.9884656743115785e+307,WorkspaceIndex="
                                          "2147483647,Height=1,DiffCoeff=2.3,Tau=1.25,Centre=0;name="
                                          "DeltaFunction,Height=1,Centre=0,constraints=(0<Height))))");
  }

  void test_setModel_with_delta_function_TwoLorenztian_correct() {
    auto algo = AlgorithmManager::Instance().create("CreateWorkspace");
    algo->initialize();
    algo->setPropertyValue("DataX", "1,2,3");
    algo->setPropertyValue("DataY", "1,2,3");
    algo->setPropertyValue("OutputWorkspace", "abc");
    algo->execute();
    ConvolutionFunctionModel model;
    model.setNumberDomains(2);
    auto pair1 = std::make_pair<std::string, int>("abc", 1);
    auto pair2 = std::make_pair<std::string, int>("abc", 2);
    auto fitResolutions = std::vector<std::pair<std::string, size_t>>();
    fitResolutions.emplace_back(pair1);
    fitResolutions.emplace_back(pair2);

    model.setModel("name=FlatBackground", fitResolutions, "(name=Lorentzian;name=Lorentzian)", "", true,
                   std::vector<double>(), false, false, 100.0);

    auto fitFunctionAsString = model.getFitFunction()->asString();
    TS_ASSERT_EQUALS(fitFunctionAsString, "composite=MultiDomainFunction,NumDeriv=true;(composite="
                                          "CompositeFunction,NumDeriv=false,$domains=i;name=FlatBackground,A0=0;("
                                          "composite=Convolution,NumDeriv=true,FixResolution=true;name="
                                          "Resolution,Workspace=abc,WorkspaceIndex=1,X=(),Y=();(name=Lorentzian,"
                                          "Amplitude=1,PeakCentre=0,FWHM=0;name=Lorentzian,Amplitude=1,"
                                          "PeakCentre=0,FWHM=0;name=DeltaFunction,Height=1,Centre=0,constraints=("
                                          "0<Height))));("
                                          "composite=CompositeFunction,NumDeriv=false,$domains=i;name="
                                          "FlatBackground,A0=0;(composite=Convolution,NumDeriv=true,"
                                          "FixResolution=true;name=Resolution,Workspace=abc,WorkspaceIndex=2,X=()"
                                          ",Y=()"
                                          ";(name=Lorentzian,Amplitude=1,PeakCentre=0,FWHM=0;name=Lorentzian,"
                                          "Amplitude=1,PeakCentre=0,FWHM=0;name=DeltaFunction,Height=1,Centre=0,"
                                          "constraints=(0<Height)))"
                                          ")");
  }

  void test_setModel_with_delta_function_TwoLorenztian_correctWithTemp() {
    auto algo = AlgorithmManager::Instance().create("CreateWorkspace");
    algo->initialize();
    algo->setPropertyValue("DataX", "1,2,3");
    algo->setPropertyValue("DataY", "1,2,3");
    algo->setPropertyValue("OutputWorkspace", "abc");
    algo->execute();
    ConvolutionFunctionModel model;
    model.setNumberDomains(2);
    auto pair1 = std::make_pair<std::string, int>("abc", 1);
    auto pair2 = std::make_pair<std::string, int>("abc", 2);
    auto fitResolutions = std::vector<std::pair<std::string, size_t>>();
    fitResolutions.emplace_back(pair1);
    fitResolutions.emplace_back(pair2);

    model.setModel("name=FlatBackground", fitResolutions, "(name=Lorentzian;name=Lorentzian)", "", true,
                   std::vector<double>(), false, true, 100.0);

    auto fitFunctionAsString = model.getFitFunction()->asString();
    TS_ASSERT_EQUALS(fitFunctionAsString, "composite=MultiDomainFunction,NumDeriv=true;(composite="
                                          "CompositeFunction,NumDeriv=false,$domains=i;name=FlatBackground,A0=0;("
                                          "composite=Convolution,NumDeriv=true,FixResolution=true;name="
                                          "Resolution,Workspace=abc,WorkspaceIndex=1,X=(),Y=();(name="
                                          "DeltaFunction,Height=1,Centre=0,constraints=(0<Height);(composite="
                                          "ProductFunction,NumDeriv="
                                          "false;name=ConvTempCorrection,Temperature=100,ties=(Temperature=100);("
                                          "name=Lorentzian,Amplitude=1,"
                                          "PeakCentre=0,FWHM=0;name=Lorentzian,Amplitude=1,PeakCentre=0,FWHM=0)))"
                                          "));(composite=CompositeFunction,NumDeriv=false,$domains=i;name="
                                          "FlatBackground,A0=0;(composite=Convolution,NumDeriv=true,"
                                          "FixResolution=true;name=Resolution,Workspace=abc,WorkspaceIndex=2,X=()"
                                          ",Y=()"
                                          ";(name=DeltaFunction,Height=1,Centre=0,constraints=(0<Height);("
                                          "composite=ProductFunction,"
                                          "NumDeriv=false;name=ConvTempCorrection,Temperature=100,ties=("
                                          "Temperature=100);(name=Lorentzian,Amplitude=1,"
                                          "PeakCentre=0,FWHM=0;name=Lorentzian,Amplitude=1,PeakCentre=0,FWHM=0)))"
                                          "))");
  }

  void test_component_prefixes_set_correctly_without_temp_correction() {
    auto algo = AlgorithmManager::Instance().create("CreateWorkspace");
    algo->initialize();
    algo->setPropertyValue("DataX", "1,2,3");
    algo->setPropertyValue("DataY", "1,2,3");
    algo->setPropertyValue("OutputWorkspace", "abc");
    algo->execute();
    ConvolutionFunctionModel model;
    model.setNumberDomains(2);
    auto pair1 = std::make_pair<std::string, int>("abc", 1);
    auto pair2 = std::make_pair<std::string, int>("abc", 2);
    auto fitResolutions = std::vector<std::pair<std::string, size_t>>();
    fitResolutions.emplace_back(pair1);
    fitResolutions.emplace_back(pair2);

    model.setModel("name=FlatBackground", fitResolutions, "(name=Lorentzian;name=Lorentzian)", "", true,
                   std::vector<double>(), false, false, 100.0);

    TS_ASSERT_EQUALS(model.backgroundPrefix().value().toStdString(), "f0.");
    TS_ASSERT_EQUALS(model.convolutionPrefix().value().toStdString(), "f1.");
    TS_ASSERT_EQUALS(model.deltaFunctionPrefix().value().toStdString(), "f1.f1.f2.");
    TS_ASSERT_EQUALS(model.peakPrefixes().value()[0].toStdString(), "f1.f1.f0.");
    TS_ASSERT_EQUALS(model.peakPrefixes().value()[1].toStdString(), "f1.f1.f1.");
  }

  void test_component_prefixes_set_correctly_with_temp_correction() {
    auto algo = AlgorithmManager::Instance().create("CreateWorkspace");
    algo->initialize();
    algo->setPropertyValue("DataX", "1,2,3");
    algo->setPropertyValue("DataY", "1,2,3");
    algo->setPropertyValue("OutputWorkspace", "abc");
    algo->execute();
    ConvolutionFunctionModel model;
    model.setNumberDomains(2);
    auto pair1 = std::make_pair<std::string, int>("abc", 1);
    auto pair2 = std::make_pair<std::string, int>("abc", 2);
    auto fitResolutions = std::vector<std::pair<std::string, size_t>>();
    fitResolutions.emplace_back(pair1);
    fitResolutions.emplace_back(pair2);

    model.setModel("name=FlatBackground", fitResolutions, "(name=Lorentzian;name=Lorentzian)", "", false,
                   std::vector<double>(), false, true, 100.0);

    TS_ASSERT_EQUALS(model.backgroundPrefix().value().toStdString(), "f0.");
    TS_ASSERT_EQUALS(model.convolutionPrefix().value().toStdString(), "f1.");
    TS_ASSERT_EQUALS(model.peakPrefixes().value()[0].toStdString(), "f1.f1.f1.f0.");
    TS_ASSERT_EQUALS(model.peakPrefixes().value()[1].toStdString(), "f1.f1.f1.f1.");
    TS_ASSERT_EQUALS(model.tempFunctionPrefix().value().toStdString(), "f1.f1.f0.");
  }

  void test_component_prefixes_if_only_temp_set() {
    auto algo = AlgorithmManager::Instance().create("CreateWorkspace");
    algo->initialize();
    algo->setPropertyValue("DataX", "1,2,3");
    algo->setPropertyValue("DataY", "1,2,3");
    algo->setPropertyValue("OutputWorkspace", "abc");
    algo->execute();
    ConvolutionFunctionModel model;
    model.setNumberDomains(2);
    auto pair1 = std::make_pair<std::string, int>("abc", 1);
    auto pair2 = std::make_pair<std::string, int>("abc", 2);
    auto fitResolutions = std::vector<std::pair<std::string, size_t>>();
    fitResolutions.emplace_back(pair1);
    fitResolutions.emplace_back(pair2);

    model.setModel("", fitResolutions, "", "", false, std::vector<double>(), false, true, 100.0);

    TS_ASSERT_EQUALS(model.convolutionPrefix().value().toStdString(), "");
    TS_ASSERT_EQUALS(model.tempFunctionPrefix().value().toStdString(), "f1.f0.");
  }

  void test_component_prefixes_one_lorenzian_temp_set() {
    auto algo = AlgorithmManager::Instance().create("CreateWorkspace");
    algo->initialize();
    algo->setPropertyValue("DataX", "1,2,3");
    algo->setPropertyValue("DataY", "1,2,3");
    algo->setPropertyValue("OutputWorkspace", "abc");
    algo->execute();
    ConvolutionFunctionModel model;
    model.setNumberDomains(2);
    auto pair1 = std::make_pair<std::string, int>("abc", 1);
    auto pair2 = std::make_pair<std::string, int>("abc", 2);
    auto fitResolutions = std::vector<std::pair<std::string, size_t>>();
    fitResolutions.emplace_back(pair1);
    fitResolutions.emplace_back(pair2);

    model.setModel("name=FlatBackground", fitResolutions, "name=Lorentzian", "", false, std::vector<double>(), false,
                   true, 100.0);
    TS_ASSERT_EQUALS(model.backgroundPrefix().value().toStdString(), "f0.");
    TS_ASSERT_EQUALS(model.convolutionPrefix().value().toStdString(), "f1.");
    TS_ASSERT_EQUALS(model.peakPrefixes().value()[0].toStdString(), "f1.f1.f1.");
    TS_ASSERT_EQUALS(model.tempFunctionPrefix().value().toStdString(), "f1.f1.f0.");
  }

  void test_component_prefixes_if_temp_and_delta_set() {
    auto algo = AlgorithmManager::Instance().create("CreateWorkspace");
    algo->initialize();
    algo->setPropertyValue("DataX", "1,2,3");
    algo->setPropertyValue("DataY", "1,2,3");
    algo->setPropertyValue("OutputWorkspace", "abc");
    algo->execute();
    ConvolutionFunctionModel model;
    model.setNumberDomains(2);
    auto pair1 = std::make_pair<std::string, int>("abc", 1);
    auto pair2 = std::make_pair<std::string, int>("abc", 2);
    auto fitResolutions = std::vector<std::pair<std::string, size_t>>();
    fitResolutions.emplace_back(pair1);
    fitResolutions.emplace_back(pair2);

    model.setModel("name=FlatBackground", fitResolutions, "name=Lorentzian", "", true, std::vector<double>(), false,
                   true, 100.0);
    TS_ASSERT_EQUALS(model.backgroundPrefix().value().toStdString(), "f0.");
    TS_ASSERT_EQUALS(model.convolutionPrefix().value().toStdString(), "f1.");
    TS_ASSERT_EQUALS(model.peakPrefixes().value()[0].toStdString(), "f1.f1.f1.f1.");
    TS_ASSERT_EQUALS(model.tempFunctionPrefix().value().toStdString(), "f1.f1.f1.f0.");
    TS_ASSERT_EQUALS(model.deltaFunctionPrefix().value().toStdString(), "f1.f1.f0.");
  }
};

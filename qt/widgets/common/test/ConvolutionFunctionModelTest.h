// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTIDWIDGETS_CONVOLUTIONFUNCTIONMODELTEST_H_
#define MANTIDWIDGETS_CONVOLUTIONFUNCTIONMODELTEST_H_

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
  ConvolutionFunctionModelTest() {
    // To make sure API is initialized properly
    FrameworkManager::Instance();
  }

  void test_empty() {
    ConvolutionFunctionModel model;
    TS_ASSERT(!model.getFitFunction());
  }

  void test_no_convolution() {
    ConvolutionFunctionModel model;
    TS_ASSERT_THROWS_EQUALS(
        model.setFunctionString("name=LinearBackground,A0=1,A1=2"),
        const std::runtime_error &e, std::string(e.what()),
        "Model doesn't contain a convolution.");
  }

  void test_no_convolution_2() {
    ConvolutionFunctionModel model;
    TS_ASSERT_THROWS_EQUALS(
        model.setFunctionString("name=LinearBackground;name=Gaussian"),
        const std::runtime_error &e, std::string(e.what()),
        "Model doesn't contain a convolution.");
  }

  void test_two_backgrounds() {
    ConvolutionFunctionModel model;
    TS_ASSERT_THROWS_EQUALS(
        model.setFunctionString(
            "name=LinearBackground;name=FlatBackground;composite=Convolution"),
        const std::runtime_error &e, std::string(e.what()),
        "Model cannot have more than one background.");
  }

  void test_wrong_resolution() {
    ConvolutionFunctionModel model;
    TS_ASSERT_THROWS_EQUALS(
        model.setFunctionString(
            "composite=Convolution;name=Gaussian;name=Lorentzian"),
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
    TS_ASSERT_THROWS_NOTHING(
        model.setFunctionString("name=LinearBackground;composite=Convolution"));
    TS_ASSERT_EQUALS(*model.backgroundPrefix(), "f0.");
    TS_ASSERT_EQUALS(*model.convolutionPrefix(), "f1.");
    TS_ASSERT_EQUALS(model.resolutionWorkspace(), "");
    TS_ASSERT_EQUALS(model.resolutionWorkspaceIndex(), 0);
    TS_ASSERT(!model.deltaFunctionPrefix());
    TS_ASSERT(!model.peakPrefixes());
  }

  void test_background_before_convolution() {
    ConvolutionFunctionModel model;
    TS_ASSERT_THROWS_NOTHING(
        model.setFunctionString("name=LinearBackground;(composite=Convolution;"
                                "name=Resolution;name=Gaussian)"));
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
    TS_ASSERT_THROWS_NOTHING(
        model.setFunctionString("(composite=Convolution;name=Resolution;name="
                                "Gaussian);name=LinearBackground"));
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
    TS_ASSERT_THROWS_NOTHING(model.setFunctionString(
        "(composite=Convolution;name=Resolution;name=Gaussian;name=Gaussian);"
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
    TS_ASSERT_THROWS_NOTHING(model.setFunctionString(
        "composite=Convolution;name=Resolution;name=Gaussian;name=Gaussian"));
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
    TS_ASSERT_THROWS_NOTHING(
        model.setFunctionString("(composite=Convolution;name=Resolution;name="
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
    TS_ASSERT_THROWS_NOTHING(model.setFunctionString(
        "composite=Convolution;name=Resolution;name=DeltaFunction"));
    TS_ASSERT(!model.backgroundPrefix());
    TS_ASSERT_EQUALS(model.convolutionPrefix()->toStdString(), "");
    TS_ASSERT_EQUALS(model.resolutionWorkspace(), "");
    TS_ASSERT_EQUALS(model.resolutionWorkspaceIndex(), 0);
    TS_ASSERT_EQUALS(model.deltaFunctionPrefix()->toStdString(), "f1.");
    TS_ASSERT(!model.peakPrefixes());
  }

  void test_two_peaks_no_background_delta() {
    ConvolutionFunctionModel model;
    TS_ASSERT_THROWS_NOTHING(
        model.setFunctionString("composite=Convolution;name=Resolution;name="
                                "Gaussian;name=Gaussian;name=DeltaFunction"));
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
    TS_ASSERT_THROWS_NOTHING(
        model.setFunctionString("(composite=Convolution;name=Resolution;name="
                                "DeltaFunction;name=Gaussian;name=Gaussian);"
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
    auto algo = FrameworkManager::Instance().createAlgorithm("CreateWorkspace");
    algo->initialize();
    algo->setPropertyValue("DataX", "1,2,3");
    algo->setPropertyValue("DataY", "1,2,3");
    algo->setPropertyValue("OutputWorkspace", "abc");
    algo->execute();
    ConvolutionFunctionModel model;
    TS_ASSERT_THROWS_NOTHING(
        model.setFunctionString("composite=Convolution;name=Resolution,"
                                "Workspace=\"abc\";name=Gaussian"));
    TS_ASSERT(!model.backgroundPrefix());
    TS_ASSERT_EQUALS(model.convolutionPrefix()->toStdString(), "");
    TS_ASSERT_EQUALS(model.resolutionWorkspace(), "abc");
    TS_ASSERT_EQUALS(model.resolutionWorkspaceIndex(), 0);
    TS_ASSERT(!model.deltaFunctionPrefix());
    TS_ASSERT(model.peakPrefixes());
    TS_ASSERT_EQUALS(model.peakPrefixes()->at(0).toStdString(), "f1.");
  }

  void test_resolution_workspace_index() {
    auto algo = FrameworkManager::Instance().createAlgorithm("CreateWorkspace");
    algo->initialize();
    algo->setPropertyValue("DataX", "1,2,3");
    algo->setPropertyValue("DataY", "1,2,3");
    algo->setPropertyValue("OutputWorkspace", "abc");
    algo->execute();
    ConvolutionFunctionModel model;
    TS_ASSERT_THROWS_NOTHING(model.setFunctionString(
        "composite=Convolution;name=Resolution,Workspace=\"abc\","
        "WorkspaceIndex=3;name=Gaussian"));
    TS_ASSERT(!model.backgroundPrefix());
    TS_ASSERT_EQUALS(model.convolutionPrefix()->toStdString(), "");
    TS_ASSERT_EQUALS(model.resolutionWorkspace(), "abc");
    TS_ASSERT_EQUALS(model.resolutionWorkspaceIndex(), 3);
    TS_ASSERT(!model.deltaFunctionPrefix());
    TS_ASSERT(model.peakPrefixes());
    TS_ASSERT_EQUALS(model.peakPrefixes()->at(0).toStdString(), "f1.");
  }

  void test_setModel_with_resolution_workspace_list_creates_correct_function() {
    auto algo = FrameworkManager::Instance().createAlgorithm("CreateWorkspace");
    algo->initialize();
    algo->setPropertyValue("DataX", "1,2,3");
    algo->setPropertyValue("DataY", "1,2,3");
    algo->setPropertyValue("OutputWorkspace", "abc");
    algo->execute();
    ConvolutionFunctionModel model;
    model.setNumberDomains(2);
    auto pair1 = std::make_pair<std::string, int>("abc", 1);
    auto pair2 = std::make_pair<std::string, int>("abc", 2);
    auto fitResolutions = std::vector<std::pair<std::string, int>>();
    fitResolutions.emplace_back(pair1);
    fitResolutions.emplace_back(pair2);

    model.setModel("", fitResolutions, "", false, std::vector<double>(), false,
                   false);

    auto fitFunctionAsString = model.getFitFunction()->asString();
    TS_ASSERT_EQUALS(
        fitFunctionAsString,
        "composite=MultiDomainFunction,NumDeriv=true;(composite=Convolution,"
        "FixResolution=true,NumDeriv=true,$domains=i;name=Resolution,Workspace="
        "abc,WorkspaceIndex=1,X=(),Y=());(composite=Convolution,FixResolution="
        "true,NumDeriv=true,$domains=i;name=Resolution,Workspace=abc,"
        "WorkspaceIndex=2,X=(),Y=())");
  }

  void test_setModel_with_delta_function_correct() {
    auto algo = FrameworkManager::Instance().createAlgorithm("CreateWorkspace");
    algo->initialize();
    algo->setPropertyValue("DataX", "1,2,3");
    algo->setPropertyValue("DataY", "1,2,3");
    algo->setPropertyValue("OutputWorkspace", "abc");
    algo->execute();
    ConvolutionFunctionModel model;
    model.setNumberDomains(2);
    auto pair1 = std::make_pair<std::string, int>("abc", 1);
    auto pair2 = std::make_pair<std::string, int>("abc", 2);
    auto fitResolutions = std::vector<std::pair<std::string, int>>();
    fitResolutions.emplace_back(pair1);
    fitResolutions.emplace_back(pair2);

    model.setModel("", fitResolutions, "", true, std::vector<double>(), false,
                   false);

    auto fitFunctionAsString = model.getFitFunction()->asString();
    std::cout << fitFunctionAsString << std::endl;
    TS_ASSERT_EQUALS(
        fitFunctionAsString,
        "composite=MultiDomainFunction,NumDeriv=true;(composite=Convolution,"
        "FixResolution=true,NumDeriv=true,$domains=i;name=Resolution,Workspace="
        "abc,WorkspaceIndex=1,X=(),Y=();name=DeltaFunction,Height=1,Centre=0);("
        "composite=Convolution,FixResolution="
        "true,NumDeriv=true,$domains=i;name=Resolution,Workspace=abc,"
        "WorkspaceIndex=2,X=(),Y=();name=DeltaFunction,Height=1,Centre=0)");
  }

  void test_setModel_with_delta_function_TeixeiraWaterSQE_correct() {
    auto algo = FrameworkManager::Instance().createAlgorithm("CreateWorkspace");
    algo->initialize();
    algo->setPropertyValue("DataX", "1,2,3");
    algo->setPropertyValue("DataY", "1,2,3");
    algo->setPropertyValue("OutputWorkspace", "abc");
    algo->execute();
    ConvolutionFunctionModel model;
    model.setNumberDomains(2);
    auto pair1 = std::make_pair<std::string, int>("abc", 1);
    auto pair2 = std::make_pair<std::string, int>("abc", 2);
    auto fitResolutions = std::vector<std::pair<std::string, int>>();
    fitResolutions.emplace_back(pair1);
    fitResolutions.emplace_back(pair2);

    model.setModel("name=FlatBackground", fitResolutions,
                   "name=TeixeiraWaterSQE", true, std::vector<double>(), false,
                   false);

    auto fitFunctionAsString = model.getFitFunction()->asString();
    std::cout << fitFunctionAsString << std::endl;
    TS_ASSERT_EQUALS(
        fitFunctionAsString,
        "composite=MultiDomainFunction,NumDeriv=true;(composite="
        "CompositeFunction,NumDeriv=false,$domains=i;name=FlatBackground,A0=0;("
        "composite=Convolution,FixResolution=true,NumDeriv=true;name="
        "Resolution,Workspace=abc,WorkspaceIndex=1,X=(),Y=();(name="
        "TeixeiraWaterSQE,Q=8.9884656743115785e+307,WorkspaceIndex=2147483647,"
        "Height=1,DiffCoeff=2.3,Tau=1.25,Centre=0;name=DeltaFunction,Height=1,"
        "Centre=0)));(composite=CompositeFunction,NumDeriv=false,$domains=i;"
        "name=FlatBackground,A0=0;(composite=Convolution,FixResolution=true,"
        "NumDeriv=true;name=Resolution,Workspace=abc,WorkspaceIndex=2,X=(),Y=()"
        ";(name=TeixeiraWaterSQE,Q=8.9884656743115785e+307,WorkspaceIndex="
        "2147483647,Height=1,DiffCoeff=2.3,Tau=1.25,Centre=0;name="
        "DeltaFunction,Height=1,Centre=0)))");
  }

  void test_setModel_with_delta_function_TwoLorenztian_correct() {
    auto algo = FrameworkManager::Instance().createAlgorithm("CreateWorkspace");
    algo->initialize();
    algo->setPropertyValue("DataX", "1,2,3");
    algo->setPropertyValue("DataY", "1,2,3");
    algo->setPropertyValue("OutputWorkspace", "abc");
    algo->execute();
    ConvolutionFunctionModel model;
    model.setNumberDomains(2);
    auto pair1 = std::make_pair<std::string, int>("abc", 1);
    auto pair2 = std::make_pair<std::string, int>("abc", 2);
    auto fitResolutions = std::vector<std::pair<std::string, int>>();
    fitResolutions.emplace_back(pair1);
    fitResolutions.emplace_back(pair2);

    model.setModel("name=FlatBackground", fitResolutions,
                   "(name=Lorentzian;name=Lorentzian)", true,
                   std::vector<double>(), false, false);

    auto fitFunctionAsString = model.getFitFunction()->asString();
    std::cout << fitFunctionAsString << std::endl;
    TS_ASSERT_EQUALS(
        fitFunctionAsString,
        "composite=MultiDomainFunction,NumDeriv=true;(composite="
        "CompositeFunction,NumDeriv=false,$domains=i;name=FlatBackground,A0=0;("
        "composite=Convolution,FixResolution=true,NumDeriv=true;name="
        "Resolution,Workspace=abc,WorkspaceIndex=1,X=(),Y=();(name=Lorentzian,"
        "Amplitude=1,PeakCentre=0,FWHM=0;name=Lorentzian,Amplitude=1,"
        "PeakCentre=0,FWHM=0;name=DeltaFunction,Height=1,Centre=0)));("
        "composite=CompositeFunction,NumDeriv=false,$domains=i;name="
        "FlatBackground,A0=0;(composite=Convolution,FixResolution=true,"
        "NumDeriv=true;name=Resolution,Workspace=abc,WorkspaceIndex=2,X=(),Y=()"
        ";(name=Lorentzian,Amplitude=1,PeakCentre=0,FWHM=0;name=Lorentzian,"
        "Amplitude=1,PeakCentre=0,FWHM=0;name=DeltaFunction,Height=1,Centre=0))"
        ")");
  }

  void test_setModel_with_delta_function_TwoLorenztian_correctWithTemp() {
    auto algo = FrameworkManager::Instance().createAlgorithm("CreateWorkspace");
    algo->initialize();
    algo->setPropertyValue("DataX", "1,2,3");
    algo->setPropertyValue("DataY", "1,2,3");
    algo->setPropertyValue("OutputWorkspace", "abc");
    algo->execute();
    ConvolutionFunctionModel model;
    model.setNumberDomains(2);
    auto pair1 = std::make_pair<std::string, int>("abc", 1);
    auto pair2 = std::make_pair<std::string, int>("abc", 2);
    auto fitResolutions = std::vector<std::pair<std::string, int>>();
    fitResolutions.emplace_back(pair1);
    fitResolutions.emplace_back(pair2);

    model.setModel("name=FlatBackground", fitResolutions,
                   "(name=Lorentzian;name=Lorentzian)", true,
                   std::vector<double>(), false, true);

    auto fitFunctionAsString = model.getFitFunction()->asString();
    std::cout << fitFunctionAsString << std::endl;
    TS_ASSERT_EQUALS(
        fitFunctionAsString,
        "composite=MultiDomainFunction,NumDeriv=true;(composite="
        "CompositeFunction,NumDeriv=false,$domains=i;name=FlatBackground,A0=0;("
        "composite=Convolution,FixResolution=true,NumDeriv=true;name="
        "Resolution,Workspace=abc,WorkspaceIndex=1,X=(),Y=();(composite="
        "ProductFunction,NumDeriv=false;name=UserFunction,Formula=x*11.606/"
        "Temp/(1-exp( "
        "-(x*11.606/"
        "Temp))),Temp=100,ties=(Temp=100);(name=Lorentzian,Amplitude=1,"
        "PeakCentre=0,FWHM=0;name=Lorentzian,Amplitude=1,PeakCentre=0,FWHM=0;"
        "name=DeltaFunction,Height=1,Centre=0))));(composite=CompositeFunction,"
        "NumDeriv=false,$domains=i;name=FlatBackground,A0=0;(composite="
        "Convolution,FixResolution=true,NumDeriv=true;name=Resolution,"
        "Workspace=abc,WorkspaceIndex=2,X=(),Y=();(composite=ProductFunction,"
        "NumDeriv=false;name=UserFunction,Formula=x*11.606/Temp/(1-exp( "
        "-(x*11.606/"
        "Temp))),Temp=100,ties=(Temp=100);(name=Lorentzian,Amplitude=1,"
        "PeakCentre=0,FWHM=0;name=Lorentzian,Amplitude=1,PeakCentre=0,FWHM=0;"
        "name=DeltaFunction,Height=1,Centre=0))))");
  }

  void test_component_prefixes_set_correctly_without_temp_correction() {
    auto algo = FrameworkManager::Instance().createAlgorithm("CreateWorkspace");
    algo->initialize();
    algo->setPropertyValue("DataX", "1,2,3");
    algo->setPropertyValue("DataY", "1,2,3");
    algo->setPropertyValue("OutputWorkspace", "abc");
    algo->execute();
    ConvolutionFunctionModel model;
    model.setNumberDomains(2);
    auto pair1 = std::make_pair<std::string, int>("abc", 1);
    auto pair2 = std::make_pair<std::string, int>("abc", 2);
    auto fitResolutions = std::vector<std::pair<std::string, int>>();
    fitResolutions.emplace_back(pair1);
    fitResolutions.emplace_back(pair2);

    model.setModel("name=FlatBackground", fitResolutions,
                   "(name=Lorentzian;name=Lorentzian)", true,
                   std::vector<double>(), false, false);

    TS_ASSERT_EQUALS(model.backgroundPrefix().value().toStdString(), "f0.");
    TS_ASSERT_EQUALS(model.convolutionPrefix().value().toStdString(), "f1.");
    TS_ASSERT_EQUALS(model.deltaFunctionPrefix().value().toStdString(),
                     "f1.f1.f2.");
    TS_ASSERT_EQUALS(model.peakPrefixes().value()[0].toStdString(),
                     "f1.f1.f0.");
    TS_ASSERT_EQUALS(model.peakPrefixes().value()[1].toStdString(),
                     "f1.f1.f1.");
  }

  void test_component_prefixes_set_correctly_with_temp_correction() {
    auto algo = FrameworkManager::Instance().createAlgorithm("CreateWorkspace");
    algo->initialize();
    algo->setPropertyValue("DataX", "1,2,3");
    algo->setPropertyValue("DataY", "1,2,3");
    algo->setPropertyValue("OutputWorkspace", "abc");
    algo->execute();
    ConvolutionFunctionModel model;
    model.setNumberDomains(2);
    auto pair1 = std::make_pair<std::string, int>("abc", 1);
    auto pair2 = std::make_pair<std::string, int>("abc", 2);
    auto fitResolutions = std::vector<std::pair<std::string, int>>();
    fitResolutions.emplace_back(pair1);
    fitResolutions.emplace_back(pair2);

    model.setModel("name=FlatBackground", fitResolutions,
                   "(name=Lorentzian;name=Lorentzian)", false,
                   std::vector<double>(), false, true);

    TS_ASSERT_EQUALS(model.backgroundPrefix().value().toStdString(), "f0.");
    TS_ASSERT_EQUALS(model.convolutionPrefix().value().toStdString(), "f1.");
    TS_ASSERT_EQUALS(model.peakPrefixes().value()[0].toStdString(),
                     "f1.f1.f1.f0.");
    TS_ASSERT_EQUALS(model.peakPrefixes().value()[1].toStdString(),
                     "f1.f1.f1.f1.");
    TS_ASSERT_EQUALS(model.tempFunctionPrefix().value().toStdString(),
                     "f1.f1.f0.");
  }

  void test_component_prefixes_if_only_temp_set() {
    auto algo = FrameworkManager::Instance().createAlgorithm("CreateWorkspace");
    algo->initialize();
    algo->setPropertyValue("DataX", "1,2,3");
    algo->setPropertyValue("DataY", "1,2,3");
    algo->setPropertyValue("OutputWorkspace", "abc");
    algo->execute();
    ConvolutionFunctionModel model;
    model.setNumberDomains(2);
    auto pair1 = std::make_pair<std::string, int>("abc", 1);
    auto pair2 = std::make_pair<std::string, int>("abc", 2);
    auto fitResolutions = std::vector<std::pair<std::string, int>>();
    fitResolutions.emplace_back(pair1);
    fitResolutions.emplace_back(pair2);

    model.setModel("", fitResolutions, "", false, std::vector<double>(), false,
                   true);

    TS_ASSERT_EQUALS(model.convolutionPrefix().value().toStdString(), "");
    TS_ASSERT_EQUALS(model.tempFunctionPrefix().value().toStdString(),
                     "f1.f0.");
  }

  void test_component_prefixes_one_lorenzian_temp_set() {
    auto algo = FrameworkManager::Instance().createAlgorithm("CreateWorkspace");
    algo->initialize();
    algo->setPropertyValue("DataX", "1,2,3");
    algo->setPropertyValue("DataY", "1,2,3");
    algo->setPropertyValue("OutputWorkspace", "abc");
    algo->execute();
    ConvolutionFunctionModel model;
    model.setNumberDomains(2);
    auto pair1 = std::make_pair<std::string, int>("abc", 1);
    auto pair2 = std::make_pair<std::string, int>("abc", 2);
    auto fitResolutions = std::vector<std::pair<std::string, int>>();
    fitResolutions.emplace_back(pair1);
    fitResolutions.emplace_back(pair2);

    model.setModel("name=FlatBackground", fitResolutions, "name=Lorentzian",
                   false, std::vector<double>(), false, true);
    TS_ASSERT_EQUALS(model.backgroundPrefix().value().toStdString(), "f0.");
    TS_ASSERT_EQUALS(model.convolutionPrefix().value().toStdString(), "f1.");
    TS_ASSERT_EQUALS(model.peakPrefixes().value()[0].toStdString(),
                     "f1.f1.f1.");
    TS_ASSERT_EQUALS(model.tempFunctionPrefix().value().toStdString(),
                     "f1.f1.f0.");
  }
};

#endif // MANTIDWIDGETS_CONVOLUTIONFUNCTIONMODELTEST_H_

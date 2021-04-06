// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "IndirectFunctionBrowser/ConvFunctionModel.h"
#include "MantidAPI/FunctionFactory.h"

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces::IDA;

class ConvFunctionModelTest : public CxxTest::TestSuite {
public:
  void setUp() override { m_model = std::make_unique<MantidQt::CustomInterfaces::IDA::ConvFunctionModel>(); }

  void tearDown() override { m_model.reset(); }

  void test_that_model_created_correctly() { TS_ASSERT(m_model); }

  void test_setFunction_correctly_handles_single_lorentzian() {
    auto fun = FunctionFactory::Instance().createInitialized(
        "composite=Convolution,FixResolution=true,NumDeriv=true;name="
        "Resolution,WorkspaceIndex=0,X=(),Y=();name=Lorentzian,Amplitude=1,"
        "PeakCentre=0,FWHM=1,constraints=(0<Amplitude,0<FWHM)");

    m_model->setFunction(fun);

    TS_ASSERT_EQUALS(m_model->getCurrentFunction()->asString(), fun->asString())
  }

  void test_setFunction_correctly_handles_Taxeira_water() {
    auto fun = FunctionFactory::Instance().createInitialized(
        "composite=Convolution,FixResolution=true,NumDeriv=true;name="
        "Resolution,WorkspaceIndex=0,X=(),Y="
        "();name=TeixeiraWaterSQE,Q=8.9884656743115785e+307,WorkspaceIndex="
        "2147483647,Height=1,DiffCoeff=2.3,Tau=1.25,Centre=0");

    m_model->setFunction(fun);

    TS_ASSERT_EQUALS(m_model->getCurrentFunction()->asString(), fun->asString())
  }

  void test_setFunction_correctly_handles_two_lorentzians() {
    auto fun = FunctionFactory::Instance().createInitialized(
        "composite=Convolution,FixResolution=true,NumDeriv=true;name="
        "Resolution,WorkspaceIndex=0,X=(),Y=();(name=Lorentzian,Amplitude=1,"
        "PeakCentre=0,FWHM=1,constraints=(0<Amplitude,0<FWHM);name=Lorentzian,"
        "Amplitude=1,PeakCentre=0,FWHM=1,constraints=(0<Amplitude,0<FWHM))");

    m_model->setFunction(fun);

    TS_ASSERT_EQUALS(m_model->getCurrentFunction()->asString(), fun->asString())
  }

  void test_setFunction_correctly_handles_one_lorentzian_and_background() {
    auto fun = FunctionFactory::Instance().createInitialized(
        "name=FlatBackground,A0=0,constraints=(0<A0);(composite=Convolution,"
        "FixResolution=true,NumDeriv=true;name=Resolution,WorkspaceIndex=0,X=()"
        ",Y=();name=Lorentzian,Amplitude=1,PeakCentre=0,FWHM=1,constraints=(0<"
        "Amplitude,0<FWHM))");

    m_model->setFunction(fun);

    TS_ASSERT_EQUALS(m_model->getCurrentFunction()->asString(), fun->asString())
  }

  void test_setFunction_correctly_handles_one_lorentzian_and_one_delta_function() {
    auto fun = FunctionFactory::Instance().createInitialized(
        "composite=Convolution,FixResolution=true,NumDeriv=true;name="
        "Resolution,WorkspaceIndex=0,X=(),Y=();(name=DeltaFunction,Height=1,"
        "Centre=0;name=Lorentzian,Amplitude=1,PeakCentre=0,FWHM=1,constraints=("
        "0<Amplitude,0<FWHM))");

    m_model->setFunction(fun);

    TS_ASSERT_EQUALS(m_model->getCurrentFunction()->asString(), fun->asString())
  }

  void test_setFunction_correctly_handles_two_lorentzian_and_one_delta_function_one_background() {
    auto fun = FunctionFactory::Instance().createInitialized(
        "name=LinearBackground,A0=0,A1=0,constraints=(0<A0);(composite="
        "Convolution,FixResolution=true,NumDeriv=true;name=Resolution,"
        "WorkspaceIndex=0,X=(),Y=();(name=DeltaFunction,Height=1,Centre=0;name="
        "Lorentzian,Amplitude=1,PeakCentre=0,FWHM=1,constraints=(0<Amplitude,0<"
        "FWHM);name=Lorentzian,Amplitude=1,PeakCentre=0,FWHM=1,constraints=(0<"
        "Amplitude,0<FWHM)))");

    m_model->setFunction(fun);

    TS_ASSERT_EQUALS(m_model->getCurrentFunction()->asString(), fun->asString())
  }

  void test_setFunction_throws_for_multiple_backgrounds() {
    auto fun = FunctionFactory::Instance().createInitialized(
        "name=LinearBackground,A0=0,A1=0,constraints=(0<A0);(composite="
        "Convolution,"
        "FixResolution=true,NumDeriv=true;name=Resolution,WorkspaceIndex=0,X=()"
        ",Y=();"
        "(name=DeltaFunction,Height=1,Centre=0;name=Lorentzian,Amplitude=1,"
        "PeakCentre=0,FWHM=1,constraints=(0<Amplitude,0<FWHM);name=Lorentzian,"
        "Amplitude=1,PeakCentre=0,FWHM=1,constraints=(0<Amplitude,0<FWHM)));"
        "name=FlatBackground,A0=0");
    TS_ASSERT_THROWS(m_model->setFunction(fun), std::runtime_error &)
  }

  void test_setFunction_does_not_throw_for_valid_temperature_function() {
    m_model->setLorentzianType(LorentzianType::OneLorentzian);
    m_model->setTempCorrection(true, 100.0);
    auto func = m_model->getFitFunction();

    m_model->setFunction(func);

    TS_ASSERT_EQUALS(m_model->getCurrentFunction()->asString(), func->asString())
    TS_ASSERT_EQUALS(m_model->getBackgroundType(), BackgroundType::None);
    TS_ASSERT_EQUALS(m_model->getLorentzianType(), LorentzianType::OneLorentzian);
  }

  void test_setFunction_does_not_throw_for_valid_temperature_function_with_delta() {
    m_model->setLorentzianType(LorentzianType::OneLorentzian);
    m_model->setTempCorrection(true, 100.0);
    m_model->setDeltaFunction(true);
    auto func = m_model->getFitFunction();

    m_model->setFunction(func);

    TS_ASSERT_EQUALS(m_model->getCurrentFunction()->asString(), func->asString())
    TS_ASSERT_EQUALS(m_model->getBackgroundType(), BackgroundType::None);
    TS_ASSERT_EQUALS(m_model->getLorentzianType(), LorentzianType::OneLorentzian);
  }

  void test_setFunction_does_not_throw_for_valid_two_lorenztian_temperature_function() {
    m_model->setLorentzianType(LorentzianType::TwoLorentzians);
    m_model->setTempCorrection(true, 100.0);
    auto func = m_model->getFitFunction();

    m_model->setFunction(func);

    TS_ASSERT_EQUALS(m_model->getCurrentFunction()->asString(), func->asString())
    TS_ASSERT_EQUALS(m_model->getBackgroundType(), BackgroundType::None);
    TS_ASSERT_EQUALS(m_model->getLorentzianType(), LorentzianType::TwoLorentzians);
  }

  void test_setFunction_does_not_throw_for_valid_two_lorenztian_temperature_function_with_delta() {
    m_model->setLorentzianType(LorentzianType::TwoLorentzians);
    m_model->setTempCorrection(true, 100.0);
    m_model->setDeltaFunction(true);
    auto func = m_model->getFitFunction();

    m_model->setFunction(func);

    TS_ASSERT_EQUALS(m_model->getCurrentFunction()->asString(), func->asString());
    TS_ASSERT_EQUALS(m_model->getBackgroundType(), BackgroundType::None);
    TS_ASSERT_EQUALS(m_model->getLorentzianType(), LorentzianType::TwoLorentzians);
  }

private:
  std::unique_ptr<MantidQt::CustomInterfaces::IDA::ConvFunctionModel> m_model;
};

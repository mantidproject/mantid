set ( SRC_FILES
	#	src/CostFuncIgnorePosPeaks.cpp
	#	src/SCDPanelErrors.cpp
	# src/ChebyshevPolynomialBackground.cpp
	src/Abragam.cpp
	src/BFGS_Minimizer.cpp
	src/BackToBackExponential.cpp
	src/BackgroundFunction.cpp
	src/BivariateNormal.cpp
	src/Bk2BkExpConvPV.cpp
	src/BoundaryConstraint.cpp
	src/Chebyshev.cpp
	src/Convolution.cpp
	src/CostFuncFitting.cpp
	src/CostFuncLeastSquares.cpp
	src/DeltaFunction.cpp
	src/DerivMinimizer.cpp
	src/DiffSphere.cpp
	src/EndErfc.cpp
	src/ExpDecay.cpp
	src/ExpDecayMuon.cpp
	src/ExpDecayOsc.cpp
	src/FRConjugateGradientMinimizer.cpp
	src/Fit.cpp
	src/Fit1D.cpp
	src/FitMW.cpp
	src/FlatBackground.cpp
	src/FuncMinimizerFactory.cpp
	src/GSLFunctions.cpp
	src/GausDecay.cpp
	src/GausOsc.cpp
	src/Gaussian.cpp
	src/Gaussian1D.cpp
	src/GaussianLinearBG1D.cpp
	src/IFuncMinimizer.cpp
	src/IkedaCarpenterPV.cpp
	src/LeBailFit.cpp
	src/LeBailFunction.cpp
	src/LevenbergMarquardtMDMinimizer.cpp
	src/LevenbergMarquardtMinimizer.cpp
	src/Linear.cpp
	src/LinearBackground.cpp
	src/LogNormal.cpp
	src/Lorentzian.cpp
	src/Lorentzian1D.cpp
	src/MultiDomainCreator.cpp
	src/MuonFInteraction.cpp
	src/PRConjugateGradientMinimizer.cpp
	src/ParDomain.cpp
	src/PlotPeakByLogValue.cpp
	src/Polynomial.cpp
	src/ProcessBackground.cpp
	src/ProductFunction.cpp
	src/ProductLinearExp.cpp
	src/ProductQuadraticExp.cpp
	src/Quadratic.cpp
	src/Resolution.cpp
	src/SeqDomain.cpp
	src/SimplexMinimizer.cpp
	src/SpecialFunctionHelper.cpp
	src/SplineBackground.cpp
	src/StaticKuboToyabe.cpp
	src/SteepestDescentMinimizer.cpp
	src/StretchExp.cpp
	src/StretchExpMuon.cpp
	src/ThermalNeutronBk2BkExpConvPV.cpp
	src/UserFunction.cpp
	src/UserFunction1D.cpp
)
                
set ( SRC_UNITY_IGNORE_FILES src/Fit1D.cpp src/GSLFunctions.cpp )

set ( INC_FILES
	#	inc/MantidCurveFitting/CostFuncIgnorePosPeaks.h
	#	inc/MantidCurveFitting/SCDPanelErrors.h
	# inc/MantidCurveFitting/ChebyshevPolynomialBackground.h
	inc/MantidCurveFitting/Abragam.h
	inc/MantidCurveFitting/BFGS_Minimizer.h
	inc/MantidCurveFitting/BackToBackExponential.h
	inc/MantidCurveFitting/BackgroundFunction.h
	inc/MantidCurveFitting/BivariateNormal.h
	inc/MantidCurveFitting/Bk2BkExpConvPV.h
	inc/MantidCurveFitting/BoundaryConstraint.h
	inc/MantidCurveFitting/Chebyshev.h
	inc/MantidCurveFitting/CompositeValues.h
	inc/MantidCurveFitting/Convolution.h
	inc/MantidCurveFitting/CostFuncFitting.h
	inc/MantidCurveFitting/CostFuncLeastSquares.h
	inc/MantidCurveFitting/DeltaFunction.h
	inc/MantidCurveFitting/DerivMinimizer.h
	inc/MantidCurveFitting/DiffSphere.h
	inc/MantidCurveFitting/DllConfig.h
	inc/MantidCurveFitting/EmptyValues.h
	inc/MantidCurveFitting/EndErfc.h
	inc/MantidCurveFitting/ExpDecay.h
	inc/MantidCurveFitting/ExpDecayMuon.h
	inc/MantidCurveFitting/ExpDecayOsc.h
	inc/MantidCurveFitting/FRConjugateGradientMinimizer.h
	inc/MantidCurveFitting/Fit.h
	inc/MantidCurveFitting/Fit1D.h
	inc/MantidCurveFitting/FitMW.h
	inc/MantidCurveFitting/FlatBackground.h
	inc/MantidCurveFitting/FuncMinimizerFactory.h
	inc/MantidCurveFitting/GSLFunctions.h
	inc/MantidCurveFitting/GSLJacobian.h
	inc/MantidCurveFitting/GSLMatrix.h
	inc/MantidCurveFitting/GSLVector.h
	inc/MantidCurveFitting/GausDecay.h
	inc/MantidCurveFitting/GausOsc.h
	inc/MantidCurveFitting/Gaussian.h
	inc/MantidCurveFitting/Gaussian1D.h
	inc/MantidCurveFitting/GaussianLinearBG1D.h
	inc/MantidCurveFitting/IFuncMinimizer.h
	inc/MantidCurveFitting/IkedaCarpenterPV.h
	inc/MantidCurveFitting/Jacobian.h
	inc/MantidCurveFitting/LeBailFit.h
	inc/MantidCurveFitting/LeBailFunction.h
	inc/MantidCurveFitting/LevenbergMarquardtMDMinimizer.h
	inc/MantidCurveFitting/LevenbergMarquardtMinimizer.h
	inc/MantidCurveFitting/Linear.h
	inc/MantidCurveFitting/LinearBackground.h
	inc/MantidCurveFitting/LogNormal.h
	inc/MantidCurveFitting/Lorentzian.h
	inc/MantidCurveFitting/Lorentzian1D.h
	inc/MantidCurveFitting/MultiDomainCreator.h
	inc/MantidCurveFitting/MuonFInteraction.h
	inc/MantidCurveFitting/PRConjugateGradientMinimizer.h
	inc/MantidCurveFitting/ParDomain.h
	inc/MantidCurveFitting/PlotPeakByLogValue.h
	inc/MantidCurveFitting/Polynomial.h
	inc/MantidCurveFitting/ProcessBackground.h
	inc/MantidCurveFitting/ProductFunction.h
	inc/MantidCurveFitting/ProductLinearExp.h
	inc/MantidCurveFitting/ProductQuadraticExp.h
	inc/MantidCurveFitting/Quadratic.h
	inc/MantidCurveFitting/Resolution.h
	inc/MantidCurveFitting/SeqDomain.h
	inc/MantidCurveFitting/SimplexMinimizer.h
	inc/MantidCurveFitting/SpecialFunctionSupport.h
	inc/MantidCurveFitting/SplineBackground.h
	inc/MantidCurveFitting/StaticKuboToyabe.h
	inc/MantidCurveFitting/SteepestDescentMinimizer.h
	inc/MantidCurveFitting/StretchExp.h
	inc/MantidCurveFitting/StretchExpMuon.h
	inc/MantidCurveFitting/ThermalNeutronBk2BkExpConvPV.h
	inc/MantidCurveFitting/UserFunction.h
	inc/MantidCurveFitting/UserFunction1D.h
)
              
set ( TEST_FILES
	# test/ChebyshevPolynomialBackgroundTest.h
	#test/SCDPanelErrorsTest.h
	test/AbragamTest.h
	test/BFGSTest.h
	test/BivariateNormalTest.h
	test/Bk2BkExpConvPVTest.h
	test/BoundaryConstraintTest.h
	test/ChebyshevTest.h
	test/CompositeFunctionTest.h
	test/ConvolutionTest.h
	test/DeltaFunctionTest.h
	test/DiffSphereTest.h
	test/EndErfcTest.h
	test/ExpDecayMuonTest.h
	test/ExpDecayOscTest.h
	test/ExpDecayTest.h
	test/FRConjugateGradientTest.h
	test/FitMWTest.h
	test/FlatBackgroundTest.h
	test/FuncMinimizerFactoryTest.h
	test/FunctionFactoryConstraintTest.h
	test/GSLMatrixTest.h
	test/GausDecayTest.h
	test/GausOscTest.h
	test/Gaussian1DTest.h
	test/GaussianTest.h
	test/IkedaCarpenterPVTest.h
	test/LeBailFitTest.h
	test/LeBailFunctionTest.h
	test/LeastSquaresTest.h
	test/LevenbergMarquardtMDTest.h
	test/LevenbergMarquardtTest.h
	test/LinearBackgroundTest.h
	test/LinearTest.h
	test/LogNormalTest.h
	test/Lorentzian1DTest.h
	test/LorentzianTest.h
	test/MultiDomainCreatorTest.h
	test/MultiDomainFunctionTest.h
	test/MuonFInteractionTest.h
	test/PRConjugateGradientTest.h
	test/PlotPeakByLogValueTest.h
	test/PolynomialTest.h
	test/ProcessBackgroundTest.h
	test/ProductFunctionTest.h
	test/ProductLinearExpTest.h
	test/ProductQuadraticExpTest.h
	test/QuadraticTest.h
	test/ResolutionTest.h
	test/SimplexTest.h
	test/SpecialFunctionSupportTest.h
	test/SplineBackgroundTest.h
	test/StaticKuboToyabeTest.h
	test/StretchExpMuonTest.h
	test/StretchExpTest.h
	test/ThermalNeutronBk2BkExpConvPVTest.h
	test/UserFunction1DTest.h
	test/UserFunctionTest.h
)


if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(CurveFitting SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Add a precompiled header where they are supported
ADD_PRECOMPILED_HEADER( inc/MantidCurveFitting/PrecompiledHeader.h MantidCurveFitting src/PrecompiledHeader.cpp SRC_FILES INC_FILES )
# Add the target for this directory
add_library ( CurveFitting ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( CurveFitting PROPERTIES OUTPUT_NAME MantidCurveFitting COMPILE_DEFINITIONS IN_MANTID_CURVEFITTING)
# Add to the 'Framework' group in VS
set_property ( TARGET CurveFitting PROPERTY FOLDER "MantidFramework" )

include_directories ( inc )

target_link_libraries ( CurveFitting ${MANTIDLIBS} ${GSL_LIBRARIES} )

if ( CXXTEST_FOUND )
  include_directories ( ../DataHandling/inc ../TestHelpers/inc )
  set ( TESTHELPER_SRCS ../TestHelpers/src/WorkspaceCreationHelper.cpp
                        ../TestHelpers/src/ComponentCreationHelper.cpp ) # Used by cxx_test_add_test
  cxxtest_add_test ( CurveFittingTest ${TEST_FILES}  )
  target_link_libraries( CurveFittingTest CurveFitting DataHandling DataObjects ${GMOCK_LIBRARIES} ${GTEST_LIBRARIES})
  add_dependencies ( FrameworkTests CurveFittingTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET CurveFittingTest PROPERTY FOLDER "UnitTests" )
endif ()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS CurveFitting ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR} )

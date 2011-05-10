set ( SRC_FILES
	src/BFGS_Minimizer.cpp
	src/BackToBackExponential.cpp
	src/BivariateNormal.cpp
	src/BackgroundFunction.cpp
	src/BoundaryConstraint.cpp
	src/Chebyshev.cpp
	src/Convolution.cpp
	src/CostFuncIgnorePosPeaks.cpp
	src/CostFuncLeastSquares.cpp
	src/CostFunctionFactory.cpp
	src/DeltaFunction.cpp
	src/ExpDecay.cpp
	src/FRConjugateGradientMinimizer.cpp
	src/Fit.cpp
	src/Fit1D.cpp
	src/FuncMinimizerFactory.cpp
	src/GSLFunctions.cpp
	src/Gaussian.cpp
	src/Gaussian1D.cpp
	src/GaussianLinearBG1D.cpp
	src/GenericFit.cpp
	src/IkedaCarpenterPV.cpp
	src/LevenbergMarquardtMinimizer.cpp
	src/Linear.cpp
	src/LinearBackground.cpp
	src/Lorentzian.cpp
	src/Lorentzian1D.cpp
	src/PRConjugateGradientMinimizer.cpp
	src/PlotPeakByLogValue.cpp
	src/ProductFunctionMW.cpp
	src/Quadratic.cpp
	src/Resolution.cpp
	src/SimplexMinimizer.cpp
	src/SpecialFunctionHelper.cpp
	src/SplineBackground.cpp
	src/UserFunction.cpp
	src/UserFunction1D.cpp
)
		
set ( SRC_UNITY_IGNORE_FILES src/Fit1D.cpp src/GSLFunctions.cpp )

set ( INC_FILES
	inc/MantidCurveFitting/BFGS_Minimizer.h
	inc/MantidCurveFitting/BackToBackExponential.h
	inc/MantidCurveFitting/BackgroundFunction.h
	inc/MantidCurveFitting/BivariateNormal.h
	inc/MantidCurveFitting/BoundaryConstraint.h
	inc/MantidCurveFitting/Chebyshev.h
	inc/MantidCurveFitting/Convolution.h
	inc/MantidCurveFitting/CostFuncIgnorePosPeaks.h
	inc/MantidCurveFitting/CostFuncLeastSquares.h
	inc/MantidCurveFitting/CostFunctionFactory.h
	inc/MantidCurveFitting/DeltaFunction.h
	inc/MantidCurveFitting/ExpDecay.h
	inc/MantidCurveFitting/FRConjugateGradientMinimizer.h
	inc/MantidCurveFitting/Fit.h
	inc/MantidCurveFitting/Fit1D.h
	inc/MantidCurveFitting/FuncMinimizerFactory.h
	inc/MantidCurveFitting/GSLFunctions.h
	inc/MantidCurveFitting/Gaussian.h
	inc/MantidCurveFitting/Gaussian1D.h
	inc/MantidCurveFitting/GaussianLinearBG1D.h
	inc/MantidCurveFitting/GenericFit.h
	inc/MantidCurveFitting/ICostFunction.h
	inc/MantidCurveFitting/IFuncMinimizer.h
	inc/MantidCurveFitting/IkedaCarpenterPV.h
	inc/MantidCurveFitting/LevenbergMarquardtMinimizer.h
	inc/MantidCurveFitting/Linear.h
	inc/MantidCurveFitting/LinearBackground.h
	inc/MantidCurveFitting/Lorentzian.h
	inc/MantidCurveFitting/Lorentzian1D.h
	inc/MantidCurveFitting/PRConjugateGradientMinimizer.h
	inc/MantidCurveFitting/PlotPeakByLogValue.h
	inc/MantidCurveFitting/ProductFunctionMW.h
	inc/MantidCurveFitting/Quadratic.h
	inc/MantidCurveFitting/Resolution.h
	inc/MantidCurveFitting/SimplexMinimizer.h
	inc/MantidCurveFitting/SpecialFunctionSupport.h
	inc/MantidCurveFitting/SplineBackground.h
	inc/MantidCurveFitting/UserFunction.h
	inc/MantidCurveFitting/UserFunction1D.h
)
	      
set ( TEST_FILES
	test/BackToBackExponentialTest.h
	test/BivariateNormalTest.h
	test/BoundaryConstraintTest.h
	test/ChebyshevTest.h
	test/CompositeFunctionTest.h
	test/ConvolutionTest.h
	test/CostFunctionFactoryTest.h
	test/ExpDecayTest.h
	test/FitTest.h
	test/FuncMinimizerFactoryTest.h
	test/FunctionFactoryTest.h
	test/FunctionTest.h
	test/Gaussian1DTest.h
	test/GaussianTest.h
	test/IkedaCarpenterPVTest.h
	test/LinearBackgroundTest.h
	test/LinearTest.h
	test/Lorentzian1DTest.h
	test/LorentzianTest.h
	test/PlotPeakByLogValueTest.h
	test/ProductFunctionMWTest.h
	test/QuadraticTest.h
	test/ResolutionTest.h
	test/SpecialFunctionSupportTest.h
	test/SplineBackgroundTest.h
	test/UserFunction1DTest.h
	test/UserFunctionTest.h
)

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(CurveFitting SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Add the target for this directory
add_library ( CurveFitting ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( CurveFitting PROPERTIES OUTPUT_NAME MantidCurveFitting )
# Add to the 'Framework' group in VS
set_property ( TARGET CurveFitting PROPERTY FOLDER "MantidFramework" )

include_directories ( inc )

target_link_libraries ( CurveFitting ${MANTIDLIBS} ${GSL_LIBRARIES} )

if ( CXXTEST_FOUND )
  include_directories ( ../DataHandling/inc ../Nexus/inc )
  cxxtest_add_test ( CurveFittingTest ${TEST_FILES} )
  target_link_libraries( CurveFittingTest CurveFitting DataHandling Nexus )
  add_dependencies ( FrameworkTests CurveFittingTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET CurveFittingTest PROPERTY FOLDER "UnitTests" )
endif ()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS CurveFitting DESTINATION ${PLUGINS_DIR} )

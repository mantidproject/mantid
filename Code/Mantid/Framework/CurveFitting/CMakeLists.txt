set ( SRC_FILES
  src/Abragam.cpp
	src/BFGS_Minimizer.cpp
	src/BackToBackExponential.cpp
	src/BackgroundFunction.cpp
	src/BivariateNormal.cpp
	src/BoundaryConstraint.cpp
	src/Chebyshev.cpp
	src/Convolution.cpp
	src/CostFuncFitting.cpp
#	src/CostFuncIgnorePosPeaks.cpp
	src/CostFuncLeastSquares.cpp
	src/DeltaFunction.cpp
	src/DiffSphere.cpp
	src/DerivMinimizer.cpp
	src/EndErfc.cpp
	src/ExpDecay.cpp
	src/ExpDecayMuon.cpp
	src/ExpDecayOsc.cpp
	src/FlatBackground.cpp
	src/FRConjugateGradientMinimizer.cpp
	src/Fit.cpp 
	src/Fit1D.cpp
	src/FitMD.cpp
	src/FitMW.cpp
	src/FuncMinimizerFactory.cpp
	src/GausDecay.cpp
	src/GausOsc.cpp
	src/Gaussian.cpp
	src/MuonFInteraction.cpp
	src/Gaussian1D.cpp
	src/GaussianLinearBG1D.cpp
	src/GSLFunctions.cpp
	src/IDomainCreator.cpp
	src/IFuncMinimizer.cpp
	src/IkedaCarpenterPV.cpp
	src/LevenbergMarquardtMinimizer.cpp
	src/LevenbergMarquardtMDMinimizer.cpp
	src/Linear.cpp
	src/LinearBackground.cpp
	src/LogNormal.cpp
	src/Lorentzian.cpp
	src/Lorentzian1D.cpp
	src/MultiDomainCreator.cpp
	src/ParDomain.cpp
	src/PRConjugateGradientMinimizer.cpp
	src/PlotPeakByLogValue.cpp
	src/ProductFunction.cpp
	src/Quadratic.cpp
	src/QuadraticBackground.cpp
	src/Resolution.cpp
#	src/SCDPanelErrors.cpp
	src/SeqDomain.cpp
	src/StaticKuboToyabe.cpp
	src/SimplexMinimizer.cpp
	src/SteepestDescentMinimizer.cpp
	src/SpecialFunctionHelper.cpp
	src/SplineBackground.cpp
	src/StretchExp.cpp
	src/StretchExpMuon.cpp
	src/UserFunction.cpp
	src/UserFunction1D.cpp
	src/UserFunctionMD.cpp
)
                
set ( SRC_UNITY_IGNORE_FILES src/Fit1D.cpp src/GSLFunctions.cpp )

set ( INC_FILES
  inc/MantidCurveFitting/Abragam.h
	inc/MantidCurveFitting/BFGS_Minimizer.h
	inc/MantidCurveFitting/BackToBackExponential.h
	inc/MantidCurveFitting/BackgroundFunction.h
	inc/MantidCurveFitting/BivariateNormal.h
	inc/MantidCurveFitting/BoundaryConstraint.h
	inc/MantidCurveFitting/Chebyshev.h
	inc/MantidCurveFitting/CompositeValues.h
	inc/MantidCurveFitting/Convolution.h
	inc/MantidCurveFitting/CostFuncFitting.h
#	inc/MantidCurveFitting/CostFuncIgnorePosPeaks.h
	inc/MantidCurveFitting/CostFuncLeastSquares.h
	inc/MantidCurveFitting/DeltaFunction.h
	inc/MantidCurveFitting/DiffSphere.h
	inc/MantidCurveFitting/DerivMinimizer.h
	inc/MantidCurveFitting/DllConfig.h
	inc/MantidCurveFitting/EmptyValues.h
	inc/MantidCurveFitting/EndErfc.h
	inc/MantidCurveFitting/ExpDecay.h
	inc/MantidCurveFitting/ExpDecayMuon.h
	inc/MantidCurveFitting/ExpDecayOsc.h
	inc/MantidCurveFitting/FlatBackground.h
	inc/MantidCurveFitting/FRConjugateGradientMinimizer.h
	inc/MantidCurveFitting/Fit.h
	inc/MantidCurveFitting/Fit1D.h
	inc/MantidCurveFitting/FitMD.h
	inc/MantidCurveFitting/FitMW.h
	inc/MantidCurveFitting/FuncMinimizerFactory.h
	inc/MantidCurveFitting/Jacobian.h
	inc/MantidCurveFitting/GSLJacobian.h
	inc/MantidCurveFitting/GSLMatrix.h
	inc/MantidCurveFitting/GSLVector.h
	inc/MantidCurveFitting/GSLFunctions.h
	inc/MantidCurveFitting/GausDecay.h
	inc/MantidCurveFitting/GausOsc.h
	inc/MantidCurveFitting/Gaussian.h
	inc/MantidCurveFitting/Gaussian1D.h
	inc/MantidCurveFitting/GaussianLinearBG1D.h
	inc/MantidCurveFitting/IDomainCreator.h
	inc/MantidCurveFitting/IFuncMinimizer.h
	inc/MantidCurveFitting/IkedaCarpenterPV.h
	inc/MantidCurveFitting/LevenbergMarquardtMinimizer.h
	inc/MantidCurveFitting/LevenbergMarquardtMDMinimizer.h
	inc/MantidCurveFitting/Linear.h
	inc/MantidCurveFitting/LinearBackground.h
	inc/MantidCurveFitting/LogNormal.h
	inc/MantidCurveFitting/Lorentzian.h
	inc/MantidCurveFitting/Lorentzian1D.h
	inc/MantidCurveFitting/MuonFInteraction.h
	inc/MantidCurveFitting/MultiDomainCreator.h
	inc/MantidCurveFitting/ParDomain.h
	inc/MantidCurveFitting/PRConjugateGradientMinimizer.h
	inc/MantidCurveFitting/PlotPeakByLogValue.h
	inc/MantidCurveFitting/ProductFunction.h
	inc/MantidCurveFitting/Quadratic.h
	inc/MantidCurveFitting/QuadraticBackground.h
	inc/MantidCurveFitting/Resolution.h
#	inc/MantidCurveFitting/SCDPanelErrors.h
	inc/MantidCurveFitting/SeqDomain.h
	inc/MantidCurveFitting/StaticKuboToyabe.h
	inc/MantidCurveFitting/SimplexMinimizer.h
	inc/MantidCurveFitting/SteepestDescentMinimizer.h
	inc/MantidCurveFitting/SpecialFunctionSupport.h
	inc/MantidCurveFitting/SplineBackground.h
	inc/MantidCurveFitting/StretchExp.h
	inc/MantidCurveFitting/StretchExpMuon.h
	inc/MantidCurveFitting/UserFunction.h
	inc/MantidCurveFitting/UserFunction1D.h
	inc/MantidCurveFitting/UserFunctionMD.h
)
              
set ( TEST_FILES
  test/AbragamTest.h
	test/DeltaFunctionTest.h
	test/DiffSphereTest.h
	test/FlatBackgroundTest.h
	test/GausOscTest.h
	test/MuonFInteractionTest.h
	#test/SCDPanelErrorsTest.h
	test/StaticKuboToyabeTest.h
	test/StretchExpMuonTest.h
	test/BFGSTest.h
	test/BivariateNormalTest.h
	test/BoundaryConstraintTest.h
	test/ChebyshevTest.h
	test/CompositeFunctionTest.h
	test/ConvolutionTest.h
	test/ExpDecayTest.h
	test/ExpDecayMuonTest.h
	test/ExpDecayOscTest.h
	test/FitMDTest.h
	test/FitMWTest.h
	test/FRConjugateGradientTest.h
	test/FuncMinimizerFactoryTest.h
	test/FunctionFactoryConstraintTest.h
	test/Gaussian1DTest.h
	test/GausDecayTest.h
	test/GaussianTest.h
	test/GSLMatrixTest.h
#	test/IkedaCarpenterPVTest.h
	test/LeastSquaresTest.h
	test/LevenbergMarquardtTest.h
	test/LevenbergMarquardtMDTest.h
	test/LinearBackgroundTest.h
	test/LinearTest.h
	test/LogNormalTest.h
	test/Lorentzian1DTest.h
	test/LorentzianTest.h
	test/MultiDomainCreatorTest.h
	test/MultiDomainFunctionTest.h
	test/PlotPeakByLogValueTest.h
	test/PRConjugateGradientTest.h
	test/ProductFunctionTest.h
	test/QuadraticBackgroundTest.h
	test/QuadraticTest.h
	test/ResolutionTest.h
	test/SimplexTest.h
	test/SpecialFunctionSupportTest.h
	test/SplineBackgroundTest.h
	test/StretchExpTest.h
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
  cxxtest_add_test ( CurveFittingTest ${TEST_FILES} )
  target_link_libraries( CurveFittingTest CurveFitting DataHandling DataObjects)
  add_dependencies ( FrameworkTests CurveFittingTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET CurveFittingTest PROPERTY FOLDER "UnitTests" )
endif ()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS CurveFitting ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR} )

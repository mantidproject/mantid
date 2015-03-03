set ( SRC_FILES
	#	src/CostFuncIgnorePosPeaks.cpp
	#	src/SCDPanelErrors.cpp
	# src/ChebyshevPolynomialBackground.cpp
	#src/RefinePowderInstrumentParameters.cpp
	src/Abragam.cpp
	src/AugmentedLagrangianOptimizer.cpp
	src/BFGS_Minimizer.cpp
	src/BSpline.cpp
	src/BackToBackExponential.cpp
	src/BackgroundFunction.cpp
	src/BivariateNormal.cpp
	src/Bk2BkExpConvPV.cpp
	src/BoundaryConstraint.cpp
	src/CalculateGammaBackground.cpp
	src/CalculateMSVesuvio.cpp
	src/Chebyshev.cpp
	src/ComptonPeakProfile.cpp
	src/ComptonProfile.cpp
	src/ComptonScatteringCountRate.cpp
	src/ConvertToYSpace.cpp
	src/Convolution.cpp
	src/ConvolveWorkspaces.cpp
	src/CostFuncFitting.cpp
	src/CostFuncLeastSquares.cpp
	src/CostFuncRwp.cpp
	src/CubicSpline.cpp
	src/DampingMinimizer.cpp
	src/DeltaFunction.cpp
	src/DerivMinimizer.cpp
	src/DiffRotDiscreteCircle.cpp
	src/DiffSphere.cpp
	src/DynamicKuboToyabe.cpp
	src/EndErfc.cpp
	src/ExpDecay.cpp
	src/ExpDecayMuon.cpp
	src/ExpDecayOsc.cpp
	src/FABADAMinimizer.cpp
	src/FRConjugateGradientMinimizer.cpp
	src/Fit.cpp
	src/Fit1D.cpp
	src/FitMW.cpp
	src/FitPowderDiffPeaks.cpp
	src/FlatBackground.cpp
	src/FullprofPolynomial.cpp
	src/FunctionDomain1DSpectrumCreator.cpp
	src/GSLFunctions.cpp
	src/GSLMatrix.cpp
	src/GausDecay.cpp
	src/GausOsc.cpp
	src/Gaussian.cpp
	src/GaussianComptonProfile.cpp
	src/GramCharlierComptonProfile.cpp
	src/IkedaCarpenterPV.cpp
	src/LeBailFit.cpp
	src/LeBailFunction.cpp
	src/LevenbergMarquardtMDMinimizer.cpp
	src/LevenbergMarquardtMinimizer.cpp
	src/LinearBackground.cpp
	src/LogNormal.cpp
	src/Lorentzian.cpp
	src/Lorentzian1D.cpp
	src/MSVesuvioHelpers.cpp
	src/MultiDomainCreator.cpp
	src/MuonFInteraction.cpp
	src/NeutronBk2BkExpConvPVoigt.cpp
	src/NormaliseByPeakArea.cpp
	src/PRConjugateGradientMinimizer.cpp
	src/ParDomain.cpp
	src/PlotPeakByLogValue.cpp
	src/Polynomial.cpp
	src/ProcessBackground.cpp
	src/ProductFunction.cpp
	src/ProductLinearExp.cpp
	src/ProductQuadraticExp.cpp
	src/PseudoVoigt.cpp
	src/Quadratic.cpp
	src/RefinePowderInstrumentParameters3.cpp
	src/ReflectivityMulf.cpp
	src/Resolution.cpp
	src/SeqDomain.cpp
	src/SeqDomainSpectrumCreator.cpp
	src/SimplexMinimizer.cpp
	src/SpecialFunctionHelper.cpp
	src/SplineBackground.cpp
	src/SplineInterpolation.cpp
	src/SplineSmoothing.cpp
	src/StaticKuboToyabe.cpp
	src/StaticKuboToyabeTimesExpDecay.cpp
	src/StaticKuboToyabeTimesGausDecay.cpp
	src/SteepestDescentMinimizer.cpp
	src/StretchExp.cpp
	src/StretchExpMuon.cpp
	src/TabulatedFunction.cpp
	src/ThermalNeutronBk2BkExpAlpha.cpp
	src/ThermalNeutronBk2BkExpBeta.cpp
	src/ThermalNeutronBk2BkExpConvPVoigt.cpp
	src/ThermalNeutronBk2BkExpSigma.cpp
	src/ThermalNeutronDtoTOFFunction.cpp
	src/UserFunction.cpp
	src/UserFunction1D.cpp
	src/VesuvioResolution.cpp
	src/Voigt.cpp
)

set ( SRC_UNITY_IGNORE_FILES src/Fit1D.cpp src/GSLFunctions.cpp )

set ( INC_FILES
	#	inc/MantidCurveFitting/CostFuncIgnorePosPeaks.h
	#	inc/MantidCurveFitting/SCDPanelErrors.h
	# inc/MantidCurveFitting/ChebyshevPolynomialBackground.h
	#inc/MantidCurveFitting/RefinePowderInstrumentParameters.h
	inc/MantidCurveFitting/Abragam.h
	inc/MantidCurveFitting/AugmentedLagrangianOptimizer.h
	inc/MantidCurveFitting/BFGS_Minimizer.h
	inc/MantidCurveFitting/BSpline.h
	inc/MantidCurveFitting/BackToBackExponential.h
	inc/MantidCurveFitting/BackgroundFunction.h
	inc/MantidCurveFitting/BivariateNormal.h
	inc/MantidCurveFitting/Bk2BkExpConvPV.h
	inc/MantidCurveFitting/BoundaryConstraint.h
	inc/MantidCurveFitting/CalculateGammaBackground.h
	inc/MantidCurveFitting/CalculateMSVesuvio.h
	inc/MantidCurveFitting/Chebyshev.h
	inc/MantidCurveFitting/ComptonPeakProfile.h
	inc/MantidCurveFitting/ComptonProfile.h
	inc/MantidCurveFitting/ComptonScatteringCountRate.h
	inc/MantidCurveFitting/ConvertToYSpace.h
	inc/MantidCurveFitting/Convolution.h
	inc/MantidCurveFitting/ConvolveWorkspaces.h
	inc/MantidCurveFitting/CostFuncFitting.h
	inc/MantidCurveFitting/CostFuncLeastSquares.h
	inc/MantidCurveFitting/CostFuncRwp.h
	inc/MantidCurveFitting/CubicSpline.h
	inc/MantidCurveFitting/DampingMinimizer.h
	inc/MantidCurveFitting/DeltaFunction.h
	inc/MantidCurveFitting/DerivMinimizer.h
	inc/MantidCurveFitting/DiffRotDiscreteCircle.h
	inc/MantidCurveFitting/DiffSphere.h
	inc/MantidCurveFitting/DllConfig.h
	inc/MantidCurveFitting/DynamicKuboToyabe.h
	inc/MantidCurveFitting/EndErfc.h
	inc/MantidCurveFitting/ExpDecay.h
	inc/MantidCurveFitting/ExpDecayMuon.h
	inc/MantidCurveFitting/ExpDecayOsc.h
	inc/MantidCurveFitting/FABADAMinimizer.h
	inc/MantidCurveFitting/FRConjugateGradientMinimizer.h
	inc/MantidCurveFitting/Fit.h
	inc/MantidCurveFitting/Fit1D.h
	inc/MantidCurveFitting/FitMW.h
	inc/MantidCurveFitting/FitPowderDiffPeaks.h
	inc/MantidCurveFitting/FlatBackground.h
	inc/MantidCurveFitting/FullprofPolynomial.h
	inc/MantidCurveFitting/FunctionDomain1DSpectrumCreator.h
	inc/MantidCurveFitting/GSLFunctions.h
	inc/MantidCurveFitting/GSLJacobian.h
	inc/MantidCurveFitting/GSLMatrix.h
	inc/MantidCurveFitting/GSLVector.h
	inc/MantidCurveFitting/GausDecay.h
	inc/MantidCurveFitting/GausOsc.h
	inc/MantidCurveFitting/Gaussian.h
	inc/MantidCurveFitting/GaussianComptonProfile.h
	inc/MantidCurveFitting/GramCharlierComptonProfile.h
	inc/MantidCurveFitting/IkedaCarpenterPV.h
	inc/MantidCurveFitting/Jacobian.h
	inc/MantidCurveFitting/LeBailFit.h
	inc/MantidCurveFitting/LeBailFunction.h
	inc/MantidCurveFitting/LevenbergMarquardtMDMinimizer.h
	inc/MantidCurveFitting/LevenbergMarquardtMinimizer.h
	inc/MantidCurveFitting/LinearBackground.h
	inc/MantidCurveFitting/LogNormal.h
	inc/MantidCurveFitting/Lorentzian.h
	inc/MantidCurveFitting/Lorentzian1D.h
	inc/MantidCurveFitting/MSVesuvioHelpers.h
	inc/MantidCurveFitting/MultiDomainCreator.h
	inc/MantidCurveFitting/MuonFInteraction.h
	inc/MantidCurveFitting/NeutronBk2BkExpConvPVoigt.h
	inc/MantidCurveFitting/NormaliseByPeakArea.h
	inc/MantidCurveFitting/PRConjugateGradientMinimizer.h
	inc/MantidCurveFitting/ParDomain.h
	inc/MantidCurveFitting/PlotPeakByLogValue.h
	inc/MantidCurveFitting/Polynomial.h
	inc/MantidCurveFitting/ProcessBackground.h
	inc/MantidCurveFitting/ProductFunction.h
	inc/MantidCurveFitting/ProductLinearExp.h
	inc/MantidCurveFitting/ProductQuadraticExp.h
	inc/MantidCurveFitting/PseudoVoigt.h
	inc/MantidCurveFitting/Quadratic.h
	inc/MantidCurveFitting/RefinePowderInstrumentParameters3.h
	inc/MantidCurveFitting/ReflectivityMulf.h
	inc/MantidCurveFitting/Resolution.h
	inc/MantidCurveFitting/SeqDomain.h
	inc/MantidCurveFitting/SeqDomainSpectrumCreator.h
	inc/MantidCurveFitting/SimplexMinimizer.h
	inc/MantidCurveFitting/SpecialFunctionSupport.h
	inc/MantidCurveFitting/SplineBackground.h
	inc/MantidCurveFitting/SplineInterpolation.h
	inc/MantidCurveFitting/SplineSmoothing.h
	inc/MantidCurveFitting/StaticKuboToyabe.h
	inc/MantidCurveFitting/StaticKuboToyabeTimesExpDecay.h
	inc/MantidCurveFitting/StaticKuboToyabeTimesGausDecay.h
	inc/MantidCurveFitting/SteepestDescentMinimizer.h
	inc/MantidCurveFitting/StretchExp.h
	inc/MantidCurveFitting/StretchExpMuon.h
	inc/MantidCurveFitting/TabulatedFunction.h
	inc/MantidCurveFitting/ThermalNeutronBk2BkExpAlpha.h
	inc/MantidCurveFitting/ThermalNeutronBk2BkExpBeta.h
	inc/MantidCurveFitting/ThermalNeutronBk2BkExpConvPVoigt.h
	inc/MantidCurveFitting/ThermalNeutronBk2BkExpSigma.h
	inc/MantidCurveFitting/ThermalNeutronDtoTOFFunction.h
	inc/MantidCurveFitting/UserFunction.h
	inc/MantidCurveFitting/UserFunction1D.h
	inc/MantidCurveFitting/VesuvioResolution.h
	inc/MantidCurveFitting/Voigt.h
)

set ( TEST_FILES
	# ChebyshevPolynomialBackgroundTest.h
	# RefinePowderInstrumentParametersTest.h
	#SCDPanelErrorsTest.h
	AbragamTest.h
	AugmentedLagrangianOptimizerTest.h
	BFGSTest.h
	BSplineTest.h
	BackToBackExponentialTest.h
	BivariateNormalTest.h
	Bk2BkExpConvPVTest.h
	BoundaryConstraintTest.h
	CalculateGammaBackgroundTest.h
	CalculateMSVesuvioTest.h
	ChebyshevTest.h
	CompositeFunctionTest.h
	ComptonPeakProfileTest.h
	ComptonProfileTest.h
	ComptonScatteringCountRateTest.h
	ConvertToYSpaceTest.h
	ConvolutionTest.h
	ConvolveWorkspacesTest.h
	CubicSplineTest.h
	DampingMinimizerTest.h
	DeltaFunctionTest.h
	DiffRotDiscreteCircleTest.h
	DiffSphereTest.h
	DynamicKuboToyabeTest.h
	EndErfcTest.h
	ExpDecayMuonTest.h
	ExpDecayOscTest.h
	ExpDecayTest.h
	FABADAMinimizerTest.h
	FRConjugateGradientTest.h
	FitMWTest.h
	FitPowderDiffPeaksTest.h
	FitTest.h
	FlatBackgroundTest.h
	FullprofPolynomialTest.h
	FunctionDomain1DSpectrumCreatorTest.h
	FunctionFactoryConstraintTest.h
	GSLMatrixTest.h
	GausDecayTest.h
	GausOscTest.h
	GaussianComptonProfileTest.h
	GaussianTest.h
	GramCharlierComptonProfileTest.h
	IPeakFunctionIntensityTest.h
	IkedaCarpenterPVTest.h
	LeBailFitTest.h
	LeBailFunctionTest.h
	LeastSquaresTest.h
	LevenbergMarquardtMDTest.h
	LevenbergMarquardtTest.h
	LinearBackgroundTest.h
	LogNormalTest.h
	Lorentzian1DTest.h
	LorentzianTest.h
	MultiDomainCreatorTest.h
	MultiDomainFunctionTest.h
	MuonFInteractionTest.h
	NeutronBk2BkExpConvPVoigtTest.h
	NormaliseByPeakAreaTest.h
	PRConjugateGradientTest.h
	PlotPeakByLogValueTest.h
	PolynomialTest.h
	ProcessBackgroundTest.h
	ProductFunctionTest.h
	ProductLinearExpTest.h
	ProductQuadraticExpTest.h
	PseudoVoigtTest.h
	QuadraticTest.h
	RefinePowderInstrumentParameters3Test.h
	ReflectivityMulfTest.h
	ResolutionTest.h
	SeqDomainSpectrumCreatorTest.h
	SimplexTest.h
	SpecialFunctionSupportTest.h
	SplineBackgroundTest.h
	SplineInterpolationTest.h
	SplineSmoothingTest.h
	StaticKuboToyabeTest.h
	StaticKuboToyabeTimesExpDecayTest.h
	StaticKuboToyabeTimesGausDecayTest.h
	StretchExpMuonTest.h
	StretchExpTest.h
	TabulatedFunctionTest.h
	ThermalNeutronBk2BkExpAlphaTest.h
	ThermalNeutronBk2BkExpBetaTest.h
	ThermalNeutronBk2BkExpConvPVoigtTest.h
	ThermalNeutronBk2BkExpSigmaTest.h
	ThermalNeutronDtoTOFFunctionTest.h
	UserFunction1DTest.h
	UserFunctionTest.h
	VesuvioResolutionTest.h
	VoigtTest.h
)


if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(CurveFitting SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Add a precompiled header where they are supported
enable_precompiled_headers( inc/MantidCurveFitting/PrecompiledHeader.h SRC_FILES )
# Add the target for this directory
add_library ( CurveFitting ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( CurveFitting PROPERTIES OUTPUT_NAME MantidCurveFitting 
                                     COMPILE_DEFINITIONS IN_MANTID_CURVEFITTING )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( CurveFitting PROPERTIES INSTALL_RPATH "@loader_path/../Contents/MacOS")
endif () 

# Add to the 'Framework' group in VS
set_property ( TARGET CurveFitting PROPERTY FOLDER "MantidFramework" )

include_directories ( inc )

target_link_libraries ( CurveFitting ${MANTIDLIBS} ${GSL_LIBRARIES} )

# Add the unit tests directory
add_subdirectory ( test )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS CurveFitting ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR} )

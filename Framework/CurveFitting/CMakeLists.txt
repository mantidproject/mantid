set(SRC_FILES
    src/Algorithms/CalculateChiSquared.cpp
    src/Algorithms/CalculateCostFunction.cpp
    src/Algorithms/ConvertToYSpace.cpp
    src/Algorithms/ConvolutionFit.cpp
    src/Algorithms/ConvolveWorkspaces.cpp
    src/Algorithms/CrystalFieldEnergies.cpp
    src/Algorithms/DoublePulseFit.cpp
    src/Algorithms/EstimateFitParameters.cpp
    src/Algorithms/EstimatePeakErrors.cpp
    src/Algorithms/EvaluateFunction.cpp
    src/Algorithms/Fit.cpp
    src/Algorithms/Fit1D.cpp
    src/Algorithms/FitPowderDiffPeaks.cpp
    src/Algorithms/IqtFit.cpp
    src/Algorithms/LeBailFit.cpp
    src/Algorithms/LeBailFunction.cpp
    src/Algorithms/NormaliseByPeakArea.cpp
    src/Algorithms/PawleyFit.cpp
    src/Algorithms/PlotPeakByLogValue.cpp
    src/Algorithms/PlotPeakByLogValueHelper.cpp
    src/Algorithms/ProfileChiSquared1D.cpp
    src/Algorithms/QENSFitSequential.cpp
    src/Algorithms/QENSFitSimultaneous.cpp
    src/Algorithms/QENSFitUtilities.cpp
    src/Algorithms/RefinePowderInstrumentParameters.cpp
    src/Algorithms/RefinePowderInstrumentParameters3.cpp
    src/Algorithms/SplineBackground.cpp
    src/Algorithms/SplineInterpolation.cpp
    src/Algorithms/SplineSmoothing.cpp
    src/Algorithms/VesuvioCalculateGammaBackground.cpp
    src/Algorithms/VesuvioCalculateMS.cpp
    src/AugmentedLagrangianOptimizer.cpp
    src/Constraints/BoundaryConstraint.cpp
    src/CostFunctions/CostFuncFitting.cpp
    src/CostFunctions/CostFuncLeastSquares.cpp
    src/CostFunctions/CostFuncRwp.cpp
    src/CostFunctions/CostFuncUnweightedLeastSquares.cpp
    src/CostFunctions/CostFuncPoisson.cpp
    src/ExcludeRangeFinder.cpp
    src/FitMW.cpp
    src/EigenComplexMatrix.cpp
    src/EigenComplexVector.cpp
    src/EigenMatrix.cpp
    src/EigenMatrixView.cpp
    src/EigenVector.cpp
    src/EigenVectorView.cpp
    src/FuncMinimizers/BFGS_Minimizer.cpp
    src/FuncMinimizers/DampedGaussNewtonMinimizer.cpp
    src/FuncMinimizers/DerivMinimizer.cpp
    src/FuncMinimizers/FABADAMinimizer.cpp
    src/FuncMinimizers/FRConjugateGradientMinimizer.cpp
    src/FuncMinimizers/LevenbergMarquardtMDMinimizer.cpp
    src/FuncMinimizers/LevenbergMarquardtMinimizer.cpp
    src/FuncMinimizers/PRConjugateGradientMinimizer.cpp
    src/FuncMinimizers/SimplexMinimizer.cpp
    src/FuncMinimizers/SteepestDescentMinimizer.cpp
    src/FuncMinimizers/TrustRegionMinimizer.cpp
    src/FunctionDomain1DSpectrumCreator.cpp
    src/Functions/Abragam.cpp
    src/Functions/ActivationK.cpp
    src/Functions/ActivationmeV.cpp
    src/Functions/AsymmetricPearsonVII.cpp
    src/Functions/BSpline.cpp
    src/Functions/BackToBackExponential.cpp
    src/Functions/BackgroundFunction.cpp
    src/Functions/BivariateNormal.cpp
    src/Functions/Bk2BkExpConvPV.cpp
    src/Functions/ChebfunBase.cpp
    src/Functions/Chebyshev.cpp
    src/Functions/ChudleyElliotSQE.cpp
    src/Functions/ComptonPeakProfile.cpp
    src/Functions/ComptonProfile.cpp
    src/Functions/ComptonScatteringCountRate.cpp
    src/Functions/Convolution.cpp
    src/Functions/CriticalPeakRelaxationRate.cpp
    src/Functions/CrystalElectricField.cpp
    src/Functions/CrystalFieldControl.cpp
    src/Functions/CrystalFieldFunction.cpp
    src/Functions/CrystalFieldPeaks.cpp
    src/Functions/CrystalFieldPeaksBase.cpp
    src/Functions/CrystalFieldMultiSpectrum.cpp
    src/Functions/CrystalFieldPeakUtils.cpp
    src/Functions/CrystalFieldSpectrum.cpp
    src/Functions/CrystalFieldHeatCapacity.cpp
    src/Functions/CrystalFieldMagnetisation.cpp
    src/Functions/CrystalFieldMoment.cpp
    src/Functions/CrystalFieldMultiSpectrum.cpp
    src/Functions/CrystalFieldPeakUtils.cpp
    src/Functions/CrystalFieldPeaks.cpp
    src/Functions/CrystalFieldPeaksBase.cpp
    src/Functions/CrystalFieldSpectrum.cpp
    src/Functions/CrystalFieldSusceptibility.cpp
    src/Functions/CubicSpline.cpp
    src/Functions/DecoupAsymPowderMagLong.cpp
    src/Functions/DecoupAsymPowderMagRot.cpp
    src/Functions/DeltaFunction.cpp
    src/Functions/DiffRotDiscreteCircle.cpp
    src/Functions/DiffSphere.cpp
    src/Functions/DynamicKuboToyabe.cpp
    src/Functions/ElasticDiffRotDiscreteCircle.cpp
    src/Functions/ElasticDiffSphere.cpp
    src/Functions/ElasticIsoRotDiff.cpp
    src/Functions/EndErfc.cpp
    src/Functions/ExpDecay.cpp
    src/Functions/ExpDecayMuon.cpp
    src/Functions/ExpDecayOsc.cpp
    src/Functions/FickDiffusionSQE.cpp
    src/Functions/FlatBackground.cpp
    src/Functions/FullprofPolynomial.cpp
    src/Functions/FunctionQDepends.cpp
    src/Functions/GausDecay.cpp
    src/Functions/GausOsc.cpp
    src/Functions/Gaussian.cpp
    src/Functions/GaussianComptonProfile.cpp
    src/Functions/GramCharlier.cpp
    src/Functions/GramCharlierComptonProfile.cpp
    src/Functions/HallRossSQE.cpp
    src/Functions/IkedaCarpenterPV.cpp
    src/Functions/InelasticDiffRotDiscreteCircle.cpp
    src/Functions/InelasticDiffSphere.cpp
    src/Functions/InelasticIsoRotDiff.cpp
    src/Functions/IsoRotDiff.cpp
    src/Functions/Keren.cpp
    src/Functions/LinearBackground.cpp
    src/Functions/LogNormal.cpp
    src/Functions/Lorentzian.cpp
    src/Functions/MagneticOrderParameter.cpp
    src/Functions/Meier.cpp
    src/Functions/MultivariateGaussianComptonProfile.cpp
    src/Functions/MuonFInteraction.cpp
    src/Functions/MuoniumDecouplingCurve.cpp
    src/Functions/NeutronBk2BkExpConvPVoigt.cpp
    src/Functions/PawleyFunction.cpp
    src/Functions/PeakParameterFunction.cpp
    src/Functions/Polynomial.cpp
    src/Functions/PowerLaw.cpp
    src/Functions/ProcessBackground.cpp
    src/Functions/ProductFunction.cpp
    src/Functions/ProductLinearExp.cpp
    src/Functions/ProductQuadraticExp.cpp
    src/Functions/PseudoVoigt.cpp
    src/Functions/Quadratic.cpp
    src/Functions/ReflectivityMulf.cpp
    src/Functions/Resolution.cpp
    src/Functions/SimpleChebfun.cpp
    src/Functions/SmoothTransition.cpp
    src/Functions/StaticKuboToyabe.cpp
    src/Functions/StaticKuboToyabeTimesExpDecay.cpp
    src/Functions/StaticKuboToyabeTimesGausDecay.cpp
    src/Functions/StaticKuboToyabeTimesStretchExp.cpp
    src/Functions/StretchExp.cpp
    src/Functions/StretchExpMuon.cpp
    src/Functions/TabulatedFunction.cpp
    src/Functions/TeixeiraWaterSQE.cpp
    src/Functions/ThermalNeutronBk2BkExpAlpha.cpp
    src/Functions/ThermalNeutronBk2BkExpBeta.cpp
    src/Functions/ThermalNeutronBk2BkExpConvPVoigt.cpp
    src/Functions/ThermalNeutronBk2BkExpSigma.cpp
    src/Functions/ThermalNeutronDtoTOFFunction.cpp
    src/Functions/UserFunction.cpp
    src/Functions/UserFunction1D.cpp
    src/Functions/VesuvioResolution.cpp
    src/Functions/Voigt.cpp
    src/Functions/ConvTempCorrection.cpp
    src/GSLFunctions.cpp
    src/GeneralDomainCreator.cpp
    src/HistogramDomainCreator.cpp
    src/IFittingAlgorithm.cpp
    src/IMWDomainCreator.cpp
    src/LatticeDomainCreator.cpp
    src/LatticeFunction.cpp
    src/MSVesuvioHelpers.cpp
    src/MultiDomainCreator.cpp
    src/MuonHelpers.cpp
    src/ParDomain.cpp
    src/ParameterEstimator.cpp
    src/RalNlls/TrustRegion.cpp
    src/RalNlls/Workspaces.cpp
    src/SeqDomain.cpp
    src/SeqDomainSpectrumCreator.cpp
    src/SpecialFunctionHelper.cpp
    src/TableWorkspaceDomainCreator.cpp
)

set(INC_FILES
    inc/MantidCurveFitting/Algorithms/CalculateChiSquared.h
    inc/MantidCurveFitting/Algorithms/CalculateCostFunction.h
    inc/MantidCurveFitting/Algorithms/ConvertToYSpace.h
    inc/MantidCurveFitting/Algorithms/ConvolutionFit.h
    inc/MantidCurveFitting/Algorithms/ConvolveWorkspaces.h
    inc/MantidCurveFitting/Algorithms/CrystalFieldEnergies.h
    inc/MantidCurveFitting/Algorithms/DoublePulseFit.h
    inc/MantidCurveFitting/Algorithms/EstimateFitParameters.h
    inc/MantidCurveFitting/Algorithms/EstimatePeakErrors.h
    inc/MantidCurveFitting/Algorithms/EvaluateFunction.h
    inc/MantidCurveFitting/Algorithms/Fit.h
    inc/MantidCurveFitting/Algorithms/Fit1D.h
    inc/MantidCurveFitting/Algorithms/FitPowderDiffPeaks.h
    inc/MantidCurveFitting/Algorithms/IqtFit.h
    inc/MantidCurveFitting/Algorithms/LeBailFit.h
    inc/MantidCurveFitting/Algorithms/LeBailFunction.h
    inc/MantidCurveFitting/Algorithms/NormaliseByPeakArea.h
    inc/MantidCurveFitting/Algorithms/PawleyFit.h
    inc/MantidCurveFitting/Algorithms/PlotPeakByLogValue.h
    inc/MantidCurveFitting/Algorithms/PlotPeakByLogValueHelper.h
    inc/MantidCurveFitting/Algorithms/ProfileChiSquared1D.h
    inc/MantidCurveFitting/Algorithms/QENSFitSequential.h
    inc/MantidCurveFitting/Algorithms/QENSFitSimultaneous.h
    inc/MantidCurveFitting/Algorithms/QENSFitUtilities.h
    inc/MantidCurveFitting/Algorithms/RefinePowderInstrumentParameters.h
    inc/MantidCurveFitting/Algorithms/RefinePowderInstrumentParameters3.h
    inc/MantidCurveFitting/Algorithms/SplineBackground.h
    inc/MantidCurveFitting/Algorithms/SplineInterpolation.h
    inc/MantidCurveFitting/Algorithms/SplineSmoothing.h
    inc/MantidCurveFitting/Algorithms/VesuvioCalculateGammaBackground.h
    inc/MantidCurveFitting/Algorithms/VesuvioCalculateMS.h
    inc/MantidCurveFitting/AugmentedLagrangianOptimizer.h
    inc/MantidCurveFitting/Constraints/BoundaryConstraint.h
    inc/MantidCurveFitting/CostFunctions/CostFuncFitting.h
    inc/MantidCurveFitting/CostFunctions/CostFuncLeastSquares.h
    inc/MantidCurveFitting/CostFunctions/CostFuncRwp.h
    inc/MantidCurveFitting/CostFunctions/CostFuncUnweightedLeastSquares.h
    inc/MantidCurveFitting/CostFunctions/CostFuncPoisson.h
    inc/MantidCurveFitting/EigenComplexMatrix.h
    inc/MantidCurveFitting/EigenComplexVector.h
    inc/MantidCurveFitting/EigenFortranDefs.h
    inc/MantidCurveFitting/EigenFortranMatrix.h
    inc/MantidCurveFitting/EigenFortranVector.h
    inc/MantidCurveFitting/EigenJacobian.h
    inc/MantidCurveFitting/EigenMatrix.h
    inc/MantidCurveFitting/EigenMatrixView.h
    inc/MantidCurveFitting/EigenVector.h
    inc/MantidCurveFitting/EigenVectorView.h
    inc/MantidCurveFitting/ExcludeRangeFinder.h
    inc/MantidCurveFitting/FitMW.h
    inc/MantidCurveFitting/FuncMinimizers/BFGS_Minimizer.h
    inc/MantidCurveFitting/FuncMinimizers/DampedGaussNewtonMinimizer.h
    inc/MantidCurveFitting/FuncMinimizers/DerivMinimizer.h
    inc/MantidCurveFitting/FuncMinimizers/FABADAMinimizer.h
    inc/MantidCurveFitting/FuncMinimizers/FRConjugateGradientMinimizer.h
    inc/MantidCurveFitting/FuncMinimizers/LevenbergMarquardtMDMinimizer.h
    inc/MantidCurveFitting/FuncMinimizers/LevenbergMarquardtMinimizer.h
    inc/MantidCurveFitting/FuncMinimizers/PRConjugateGradientMinimizer.h
    inc/MantidCurveFitting/FuncMinimizers/SimplexMinimizer.h
    inc/MantidCurveFitting/FuncMinimizers/SteepestDescentMinimizer.h
    inc/MantidCurveFitting/FuncMinimizers/TrustRegionMinimizer.h
    inc/MantidCurveFitting/FunctionDomain1DSpectrumCreator.h
    inc/MantidCurveFitting/Functions/Abragam.h
    inc/MantidCurveFitting/Functions/ActivationK.h
    inc/MantidCurveFitting/Functions/ActivationmeV.h
    inc/MantidCurveFitting/Functions/AsymmetricPearsonVII.h
    inc/MantidCurveFitting/Functions/BSpline.h
    inc/MantidCurveFitting/Functions/BackToBackExponential.h
    inc/MantidCurveFitting/Functions/BackgroundFunction.h
    inc/MantidCurveFitting/Functions/BivariateNormal.h
    inc/MantidCurveFitting/Functions/Bk2BkExpConvPV.h
    inc/MantidCurveFitting/Functions/ChebfunBase.h
    inc/MantidCurveFitting/Functions/Chebyshev.h
    inc/MantidCurveFitting/Functions/ChudleyElliotSQE.h
    inc/MantidCurveFitting/Functions/ComptonPeakProfile.h
    inc/MantidCurveFitting/Functions/ComptonProfile.h
    inc/MantidCurveFitting/Functions/ComptonScatteringCountRate.h
    inc/MantidCurveFitting/Functions/Convolution.h
    inc/MantidCurveFitting/Functions/CriticalPeakRelaxationRate.h
    inc/MantidCurveFitting/Functions/CrystalElectricField.h
    inc/MantidCurveFitting/Functions/CrystalFieldControl.h
    inc/MantidCurveFitting/Functions/CrystalFieldFunction.h
    inc/MantidCurveFitting/Functions/CrystalFieldHeatCapacity.h
    inc/MantidCurveFitting/Functions/CrystalFieldMagnetisation.h
    inc/MantidCurveFitting/Functions/CrystalFieldMoment.h
    inc/MantidCurveFitting/Functions/CrystalFieldMultiSpectrum.h
    inc/MantidCurveFitting/Functions/CrystalFieldPeakUtils.h
    inc/MantidCurveFitting/Functions/CrystalFieldPeaks.h
    inc/MantidCurveFitting/Functions/CrystalFieldPeaksBase.h
    inc/MantidCurveFitting/Functions/CrystalFieldSpectrum.h
    inc/MantidCurveFitting/Functions/CrystalFieldSusceptibility.h
    inc/MantidCurveFitting/Functions/CubicSpline.h
    inc/MantidCurveFitting/Functions/DecoupAsymPowderMagLong.h
    inc/MantidCurveFitting/Functions/DecoupAsymPowderMagRot.h
    inc/MantidCurveFitting/Functions/DeltaFunction.h
    inc/MantidCurveFitting/Functions/DiffRotDiscreteCircle.h
    inc/MantidCurveFitting/Functions/DiffSphere.h
    inc/MantidCurveFitting/Functions/DynamicKuboToyabe.h
    inc/MantidCurveFitting/Functions/ElasticDiffRotDiscreteCircle.h
    inc/MantidCurveFitting/Functions/ElasticDiffSphere.h
    inc/MantidCurveFitting/Functions/ElasticIsoRotDiff.h
    inc/MantidCurveFitting/Functions/EndErfc.h
    inc/MantidCurveFitting/Functions/ExpDecay.h
    inc/MantidCurveFitting/Functions/ExpDecayMuon.h
    inc/MantidCurveFitting/Functions/ExpDecayOsc.h
    inc/MantidCurveFitting/Functions/FickDiffusionSQE.h
    inc/MantidCurveFitting/Functions/FlatBackground.h
    inc/MantidCurveFitting/Functions/FullprofPolynomial.h
    inc/MantidCurveFitting/Functions/FunctionQDepends.h
    inc/MantidCurveFitting/Functions/GausDecay.h
    inc/MantidCurveFitting/Functions/GausOsc.h
    inc/MantidCurveFitting/Functions/Gaussian.h
    inc/MantidCurveFitting/Functions/GaussianComptonProfile.h
    inc/MantidCurveFitting/Functions/GramCharlier.h
    inc/MantidCurveFitting/Functions/GramCharlierComptonProfile.h
    inc/MantidCurveFitting/Functions/HallRossSQE.h
    inc/MantidCurveFitting/Functions/IkedaCarpenterPV.h
    inc/MantidCurveFitting/Functions/InelasticDiffRotDiscreteCircle.h
    inc/MantidCurveFitting/Functions/InelasticDiffSphere.h
    inc/MantidCurveFitting/Functions/InelasticIsoRotDiff.h
    inc/MantidCurveFitting/Functions/IsoRotDiff.h
    inc/MantidCurveFitting/Functions/Keren.h
    inc/MantidCurveFitting/Functions/LinearBackground.h
    inc/MantidCurveFitting/Functions/LogNormal.h
    inc/MantidCurveFitting/Functions/Lorentzian.h
    inc/MantidCurveFitting/Functions/MagneticOrderParameter.h
    inc/MantidCurveFitting/Functions/Meier.h
    inc/MantidCurveFitting/Functions/MultivariateGaussianComptonProfile.h
    inc/MantidCurveFitting/Functions/MuonFInteraction.h
    inc/MantidCurveFitting/Functions/MuoniumDecouplingCurve.h
    inc/MantidCurveFitting/Functions/NeutronBk2BkExpConvPVoigt.h
    inc/MantidCurveFitting/Functions/PawleyFunction.h
    inc/MantidCurveFitting/Functions/PeakParameterFunction.h
    inc/MantidCurveFitting/Functions/Polynomial.h
    inc/MantidCurveFitting/Functions/PowerLaw.h
    inc/MantidCurveFitting/Functions/ProcessBackground.h
    inc/MantidCurveFitting/Functions/ProductFunction.h
    inc/MantidCurveFitting/Functions/ProductLinearExp.h
    inc/MantidCurveFitting/Functions/ProductQuadraticExp.h
    inc/MantidCurveFitting/Functions/PseudoVoigt.h
    inc/MantidCurveFitting/Functions/Quadratic.h
    inc/MantidCurveFitting/Functions/ReflectivityMulf.h
    inc/MantidCurveFitting/Functions/Resolution.h
    inc/MantidCurveFitting/Functions/SimpleChebfun.h
    inc/MantidCurveFitting/Functions/SmoothTransition.h
    inc/MantidCurveFitting/Functions/StaticKuboToyabe.h
    inc/MantidCurveFitting/Functions/StaticKuboToyabeTimesExpDecay.h
    inc/MantidCurveFitting/Functions/StaticKuboToyabeTimesGausDecay.h
    inc/MantidCurveFitting/Functions/StaticKuboToyabeTimesStretchExp.h
    inc/MantidCurveFitting/Functions/StretchExp.h
    inc/MantidCurveFitting/Functions/StretchExpMuon.h
    inc/MantidCurveFitting/Functions/TabulatedFunction.h
    inc/MantidCurveFitting/Functions/TeixeiraWaterSQE.h
    inc/MantidCurveFitting/Functions/ThermalNeutronBk2BkExpAlpha.h
    inc/MantidCurveFitting/Functions/ThermalNeutronBk2BkExpBeta.h
    inc/MantidCurveFitting/Functions/ThermalNeutronBk2BkExpConvPVoigt.h
    inc/MantidCurveFitting/Functions/ThermalNeutronBk2BkExpSigma.h
    inc/MantidCurveFitting/Functions/ThermalNeutronDtoTOFFunction.h
    inc/MantidCurveFitting/Functions/UserFunction.h
    inc/MantidCurveFitting/Functions/UserFunction1D.h
    inc/MantidCurveFitting/Functions/VesuvioResolution.h
    inc/MantidCurveFitting/Functions/Voigt.h
    inc/MantidCurveFitting/Functions/ConvTempCorrection.h
    inc/MantidCurveFitting/GSLFunctions.h
    inc/MantidCurveFitting/GeneralDomainCreator.h
    inc/MantidCurveFitting/HalfComplex.h
    inc/MantidCurveFitting/HistogramDomainCreator.h
    inc/MantidCurveFitting/IFittingAlgorithm.h
    inc/MantidCurveFitting/IMWDomainCreator.h
    inc/MantidCurveFitting/Jacobian.h
    inc/MantidCurveFitting/LatticeDomainCreator.h
    inc/MantidCurveFitting/LatticeFunction.h
    inc/MantidCurveFitting/MSVesuvioHelpers.h
    inc/MantidCurveFitting/MultiDomainCreator.h
    inc/MantidCurveFitting/MuonHelpers.h
    inc/MantidCurveFitting/ParDomain.h
    inc/MantidCurveFitting/ParameterEstimator.h
    inc/MantidCurveFitting/RalNlls/TrustRegion.h
    inc/MantidCurveFitting/RalNlls/Workspaces.h
    inc/MantidCurveFitting/SeqDomain.h
    inc/MantidCurveFitting/SeqDomainSpectrumCreator.h
    inc/MantidCurveFitting/SpecialFunctionSupport.h
    inc/MantidCurveFitting/TableWorkspaceDomainCreator.h
)

set(TEST_FILES
    Algorithms/CalculateChiSquaredTest.h
    Algorithms/CalculateCostFunctionTest.h
    Algorithms/ConvertToYSpaceTest.h
    Algorithms/ConvolutionFitSequentialTest.h
    Algorithms/ConvolveWorkspacesTest.h
    Algorithms/DoublePulseFitTest.h
    Algorithms/EstimateFitParametersTest.h
    Algorithms/EstimatePeakErrorsTest.h
    Algorithms/EvaluateFunctionTest.h
    Algorithms/FitPowderDiffPeaksTest.h
    Algorithms/FitTest.h
    Algorithms/LeBailFitTest.h
    Algorithms/LeBailFunctionTest.h
    Algorithms/NormaliseByPeakAreaTest.h
    Algorithms/PawleyFitTest.h
    Algorithms/PlotPeakByLogValueTest.h
    Algorithms/PlotPeakByLogValueHelperTest.h
    Algorithms/ProfileChiSquared1DTest.h
    Algorithms/QENSFitSequentialTest.h
    Algorithms/QENSFitSimultaneousTest.h
    Algorithms/RefinePowderInstrumentParameters3Test.h
    Algorithms/RefinePowderInstrumentParametersTest.h
    Algorithms/SeqDomainSpectrumCreatorTest.h
    Algorithms/SplineBackgroundTest.h
    Algorithms/SplineInterpolationTest.h
    Algorithms/SplineSmoothingTest.h
    Algorithms/VesuvioCalculateGammaBackgroundTest.h
    Algorithms/VesuvioCalculateMSTest.h
    AugmentedLagrangianOptimizerTest.h
    CompositeFunctionTest.h
    Constraints/BoundaryConstraintTest.h
    CostFunctions/CostFuncFittingTest.h
    CostFunctions/CostFuncUnweightedLeastSquaresTest.h
    CostFunctions/LeastSquaresTest.h
    CostFuncPoissonTest.h
    EigenComplexMatrixTest.h
    EigenComplexVectorTest.h
    EigenFortranMatrixTest.h
    EigenFortranVectorTest.h
    EigenJacobianTest.h
    EigenMatrixTest.h
    EigenToGSLTest.h
    EigenVectorTest.h
    EigenViewTest.h
    FitMWTest.h
    FuncMinimizers/BFGSTest.h
    FuncMinimizers/DampedGaussNewtonMinimizerTest.h
    FuncMinimizers/ErrorMessagesTest.h
    FuncMinimizers/FABADAMinimizerTest.h
    FuncMinimizers/FRConjugateGradientTest.h
    FuncMinimizers/LevenbergMarquardtMDTest.h
    FuncMinimizers/LevenbergMarquardtTest.h
    FuncMinimizers/PRConjugateGradientTest.h
    FuncMinimizers/SimplexTest.h
    FuncMinimizers/TrustRegionMinimizerTest.h
    FunctionDomain1DSpectrumCreatorTest.h
    FunctionFactoryConstraintTest.h
    FunctionParameterDecoratorFitTest.h
    Functions/AbragamTest.h
    Functions/ActivationKTest.h
    Functions/ActivationmeVTest.h
    Functions/AsymmetricPearsonVIITest.h
    Functions/BSplineTest.h
    Functions/BackToBackExponentialTest.h
    Functions/BivariateNormalTest.h
    Functions/Bk2BkExpConvPVTest.h
    Functions/ChebfunBaseTest.h
    Functions/ChebyshevTest.h
    Functions/ChudleyElliotSQETest.h
    Functions/ComptonPeakProfileTest.h
    Functions/ComptonProfileTest.h
    Functions/ComptonScatteringCountRateTest.h
    Functions/ConvolutionTest.h
    Functions/CriticalPeakRelaxationRateTest.h
    Functions/CrystalFieldControlTest.h
    Functions/CrystalFieldEnergiesTest.h
    Functions/CrystalFieldFunctionTest.h
    Functions/CrystalFieldHeatCapacityTest.h
    Functions/CrystalFieldMagnetisationTest.h
    Functions/CrystalFieldMomentTest.h
    Functions/CrystalFieldMultiSpectrumTest.h
    Functions/CrystalFieldPeaksTest.h
    Functions/CrystalFieldSpectrumTest.h
    Functions/CrystalFieldSusceptibilityTest.h
    Functions/CrystalFieldTest.h
    Functions/CubicSplineTest.h
    Functions/DecoupAsymPowderMagLongTest.h
    Functions/DecoupAsymPowderMagRotTest.h
    Functions/DeltaFunctionTest.h
    Functions/DiffRotDiscreteCircleTest.h
    Functions/DiffSphereTest.h
    Functions/DynamicKuboToyabeTest.h
    Functions/ElasticIsoRotDiffTest.h
    Functions/EndErfcTest.h
    Functions/ExpDecayMuonTest.h
    Functions/ExpDecayOscTest.h
    Functions/ExpDecayTest.h
    Functions/FickDiffusionSQETest.h
    Functions/FlatBackgroundTest.h
    Functions/FullprofPolynomialTest.h
    Functions/FunctionQDependsTest.h
    Functions/GausDecayTest.h
    Functions/GausOscTest.h
    Functions/GaussianComptonProfileTest.h
    Functions/GaussianTest.h
    Functions/GramCharlierComptonProfileTest.h
    Functions/HallRossSQETest.h
    Functions/IkedaCarpenterPVTest.h
    Functions/InelasticIsoRotDiffTest.h
    Functions/IsoRotDiffTest.h
    Functions/KerenTest.h
    Functions/LinearBackgroundTest.h
    Functions/LogNormalTest.h
    Functions/LorentzianTest.h
    Functions/MagneticOrderParameterTest.h
    Functions/MeierTest.h
    Functions/MultivariateGaussianComptonProfileTest.h
    Functions/MuonFInteractionTest.h
    Functions/MuoniumDecouplingCurveTest.h
    Functions/NeutronBk2BkExpConvPVoigtTest.h
    Functions/PawleyFunctionTest.h
    Functions/PeakParameterFunctionTest.h
    Functions/PolynomialTest.h
    Functions/PowerLawTest.h
    Functions/ProcessBackgroundTest.h
    Functions/ProductFunctionTest.h
    Functions/ProductLinearExpTest.h
    Functions/ProductQuadraticExpTest.h
    Functions/PseudoVoigtTest.h
    Functions/QuadraticTest.h
    Functions/ReflectivityMulfTest.h
    Functions/ResolutionTest.h
    Functions/SimpleChebfunTest.h
    Functions/SmoothTransitionTest.h
    Functions/StaticKuboToyabeTest.h
    Functions/StaticKuboToyabeTimesExpDecayTest.h
    Functions/StaticKuboToyabeTimesGausDecayTest.h
    Functions/StaticKuboToyabeTimesStretchExpTest.h
    Functions/StretchExpMuonTest.h
    Functions/StretchExpTest.h
    Functions/TabulatedFunctionTest.h
    Functions/TeixeiraWaterSQETest.h
    Functions/ThermalNeutronBk2BkExpAlphaTest.h
    Functions/ThermalNeutronBk2BkExpBetaTest.h
    Functions/ThermalNeutronBk2BkExpConvPVoigtTest.h
    Functions/ThermalNeutronBk2BkExpSigmaTest.h
    Functions/ThermalNeutronDtoTOFFunctionTest.h
    Functions/UserFunction1DTest.h
    Functions/UserFunctionTest.h
    Functions/VesuvioResolutionTest.h
    Functions/VoigtTest.h
    Functions/ConvTempCorrectionTest.h
    GeneralDomainCreatorTest.h
    GramCharlierTest.h
    HistogramDomainCreatorTest.h
    IPeakFunctionCentreParameterNameTest.h
    IPeakFunctionIntensityTest.h
    LatticeDomainCreatorTest.h
    LatticeFunctionTest.h
    MultiDomainCreatorTest.h
    MultiDomainFunctionTest.h
    MuonHelperTest.h
    ParameterEstimatorTest.h
    RalNlls/NLLSTest.h
    SpecialFunctionSupportTest.h
    TableWorkspaceDomainCreatorTest.h
)

if(COVERAGE)
  foreach(loop_var ${SRC_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

# Add a precompiled header where they are supported
enable_precompiled_headers(inc/MantidCurveFitting/PrecompiledHeader.h SRC_FILES)
# Add the target for this directory
add_library(CurveFitting ${SRC_FILES} ${INC_FILES})
add_library(Mantid::CurveFitting ALIAS CurveFitting)

target_include_directories(
  CurveFitting PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc> $<INSTALL_INTERFACE:include/Mantid>
)

# Set the name of the generated library
set_target_properties(CurveFitting PROPERTIES OUTPUT_NAME MantidCurveFitting COMPILE_DEFINITIONS IN_MANTID_CURVEFITTING)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_target_properties(CurveFitting PROPERTIES INSTALL_RPATH "@loader_path/../Contents/MacOS")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(CurveFitting PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

# Add to the 'Framework' group in VS
set_property(TARGET CurveFitting PROPERTY FOLDER "MantidFramework")

target_link_libraries(
  CurveFitting
  PUBLIC Mantid::API Mantid::Kernel Mantid::DataObjects
  PRIVATE Mantid::HistogramData Mantid::Geometry
)

# Add the unit tests directory
add_subdirectory(test)

# Auto-generate exports header
target_include_directories(
  CurveFitting PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/Framework/CurveFitting> $<INSTALL_INTERFACE:include/Mantid>
)
generate_mantid_export_header(CurveFitting FALSE)

# Installation settings
mtd_install_framework_lib(TARGETS CurveFitting PLUGIN_LIB)

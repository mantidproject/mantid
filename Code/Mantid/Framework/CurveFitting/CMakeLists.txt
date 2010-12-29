set ( SRC_FILES src/BackgroundFunction.cpp
      src/BackToBackExponential.cpp src/BFGS_Minimizer.cpp
      src/BoundaryConstraint.cpp src/Chebyshev.cpp src/Convolution.cpp
      src/CostFuncIgnorePosPeaks.cpp src/CostFuncLeastSquares.cpp
      src/CostFunctionFactory.cpp src/DeltaFunction.cpp
      src/ExpDecay.cpp src/Fit1D.cpp src/Fit.cpp
      src/FRConjugateGradientMinimizer.cpp
      src/FuncMinimizerFactory.cpp src/Gaussian1D.cpp src/Gaussian.cpp
      src/GaussianLinearBG1D.cpp src/GSLFunctions.cpp
      src/IkedaCarpenterPV.cpp src/LevenbergMarquardtMinimizer.cpp
      src/LinearBackground.cpp src/Linear.cpp src/Lorentzian1D.cpp
      src/Lorentzian.cpp src/PlotPeakByLogValue.cpp
      src/PRConjugateGradientMinimizer.cpp src/Quadratic.cpp
      src/Resolution.cpp src/SimplexMinimizer.cpp
      src/SpecialFunctionHelper.cpp src/SplineBackground.cpp
      src/UserFunction1D.cpp src/UserFunction.cpp )

set ( INC_FILES inc/MantidCurveFitting/BackgroundFunction.h
      inc/MantidCurveFitting/BackToBackExponential.h
      inc/MantidCurveFitting/BFGS_Minimizer.h
      inc/MantidCurveFitting/BoundaryConstraint.h
      inc/MantidCurveFitting/Chebyshev.h
      inc/MantidCurveFitting/Convolution.h
      inc/MantidCurveFitting/CostFuncIgnorePosPeaks.h
      inc/MantidCurveFitting/CostFuncLeastSquares.h
      inc/MantidCurveFitting/CostFunctionFactory.h
      inc/MantidCurveFitting/DeltaFunction.h
      inc/MantidCurveFitting/ExpDecay.h inc/MantidCurveFitting/Fit1D.h
      inc/MantidCurveFitting/Fit.h
      inc/MantidCurveFitting/FRConjugateGradientMinimizer.h
      inc/MantidCurveFitting/FuncMinimizerFactory.h
      inc/MantidCurveFitting/Gaussian1D.h
      inc/MantidCurveFitting/Gaussian.h
      inc/MantidCurveFitting/GaussianLinearBG1D.h
      inc/MantidCurveFitting/GSLFunctions.h
      inc/MantidCurveFitting/ICostFunction.h
      inc/MantidCurveFitting/IFuncMinimizer.h
      inc/MantidCurveFitting/IkedaCarpenterPV.h
      inc/MantidCurveFitting/LevenbergMarquardtMinimizer.h
      inc/MantidCurveFitting/LinearBackground.h
      inc/MantidCurveFitting/Linear.h
      inc/MantidCurveFitting/Lorentzian1D.h
      inc/MantidCurveFitting/Lorentzian.h
      inc/MantidCurveFitting/PlotPeakByLogValue.h
      inc/MantidCurveFitting/PRConjugateGradientMinimizer.h
      inc/MantidCurveFitting/Quadratic.h
      inc/MantidCurveFitting/Resolution.h
      inc/MantidCurveFitting/SimplexMinimizer.h
      inc/MantidCurveFitting/SpecialFunctionSupport.h
      inc/MantidCurveFitting/SplineBackground.h
      inc/MantidCurveFitting/UserFunction1D.h
      inc/MantidCurveFitting/UserFunction.h )

# Add the target for this directory
add_library ( CurveFitting ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( CurveFitting PROPERTIES OUTPUT_NAME MantidCurveFitting )
# Add to the 'Framework' group in VS
set_property ( TARGET CurveFitting PROPERTY FOLDER "Framework" )

include_directories ( inc )

target_link_libraries ( CurveFitting ${MANTIDLIBS} ${GSL_LIBRARIES} )
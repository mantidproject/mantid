# -----------------------------------------------------
#  Runs PlotAssymetryByLogValue algorithm
#  and plots the output in MantidPlot
#------------------------------------------------------

# Execute the algorithm
alg = PlotAsymmetryByLogValueDialog()

# Get the output workspace
ws = alg.getPropertyValue("OutputWorkspace")

g1 = plotSpectrum(ws,0)
g1.activeLayer().setCurveTitle(0,"Difference")

green = int(alg.getPropertyValue("Green"))

# This should check if Green is set
if green < 1000:
	mergePlots(g1, plotSpectrum(ws,1))
	g1.activeLayer().setCurveTitle(1,"Red")
	mergePlots(g1, plotSpectrum(ws,2))
	g1.activeLayer().setCurveTitle(2,"Green")
	mergePlots(g1, plotSpectrum(ws,3))
	g1.activeLayer().setCurveTitle(3,"Sum")

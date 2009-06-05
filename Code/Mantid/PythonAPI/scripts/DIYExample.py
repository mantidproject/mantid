# Please make sure that the "GEM38370.raw" file is in the "C:/MantidInstall/Data" directory

#For convenience set a variable pointing to the Mantid path 
path = "C:/MantidInstall/"

# Step (1)
LoadRaw(Filename=path+"Data/GEM38370.raw", OutputWorkspace="GEM38370")

#Step(2). Import the matrix starting at 200
gridGEM = importMatrixWorkspace("GEM38370",200)

# Step(3)  - A contour plot with colour fill
gridGEM.plotGraph2D(Layer.ColorMap)

# Step(4) - Rebin the GEM data
Rebin(InputWorkspace="GEM38370", OutputWorkspace="MyWorkspace", params="0,200,20000")

# Step (5) - Create a 1D plot of a spectrum. In python this automatically creates the corresponding matrix
plotSpectrum("MyWorkspace", 9)

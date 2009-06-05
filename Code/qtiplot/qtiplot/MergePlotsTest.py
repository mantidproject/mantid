root = "../../../../Test/Data/"
resultWS1="48098"
LoadRKH(root+"48098.Q",resultWS1, "Wavelength")
resultWS2="48094"
LoadRKH(root+"48094.Q",resultWS2, "Wavelength")

# Plot a spectrum from both files (this automatically imports the MantidMatrix but 
# it is minimised by default
g1 = plotSpectrum(resultWS1, 0)
g2 = plotSpectrum(resultWS2,0)

mergePlots(g1,g2)


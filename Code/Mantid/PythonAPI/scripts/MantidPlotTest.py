import MantidPythonAPI
mtd = MantidPythonAPI.FrameworkManager()

setWorkingDirectory("C:/MantidInstall")

mtd.execute("LoadRaw","Data/MAR11060.RAW;test",-1)
mtd.execute("ConvertUnits","test;converted;dSpacing",-1)
mtd.execute("Rebin","converted;rebinned;0.1,0.001,5",-1)
#clear up intermediate workspaces
mtd.deleteWorkspace("test")
mtd.deleteWorkspace("converted")
#extract the one we want
w=mtd.getMatrixWorkspace('rebinned')
print w.getNumberHistograms()












# Instrument Window test script
setWorkingDirectory("/home/dmn58364/Mantid/trunk/Test/Data")

LoadRaw("LOQ sans configuration/LOQ48094.raw","LOQTest")

insView = getInstrumentView("LOQTest")

insView.changeColorMap("../../Code/qtiplot/colormaps/_standard.map")

insView.setColorMapRange(0.,195)
# Or can set a minimum and maximum separately
#insView.setColorMapMinValue(1.)
#insView.setColorMapMaxValue(10.)

# Alter the binning range
insView.setBinRange(10000,15000)

insView.showWindow()


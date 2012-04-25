# Perform some algorithms
LoadRaw(Filename="GEM40979.raw",OutputWorkspace="GEM40979")
AlignDetectors("GEM40979","aligned",CalibrationFile="offsets_2006_cycle064.cal")
mtd.deleteWorkspace("GEM40979")
DiffractionFocussing("aligned","focussed","offsets_2006_cycle064.cal")
mtd.deleteWorkspace("aligned")
FindPeaks("focussed","peakslist")

peaks = mtd.getTableWorkspace("peakslist")

print "Number of Columns: " + str(peaks.getColumnCount()) + " , Number of rows: "+ str(peaks.getRowCount())

colNames = peaks.getColumnNames()
for i in colNames:
	print i

for i in range(peaks.getRowCount()):
	print "Spectrum %d has peak at pos %f" % (peaks.getInt("spectrum",i), peaks.getDouble("centre",i))


# The directory of the data files
setWorkingDirectory("/home/dmn58364/Mantid/trunk/Test/Data")

# Perform some algorithms
LoadRaw(Filename="GEM40979.raw",OutputWorkspace="GEM40979")
AlignDetectors("GEM40979","aligned",CalibrationFile="offsets_2006_cycle064.cal")
mtd.deleteWorkspace("GEM40979")
DiffractionFocussing("aligned","focussed","offsets_2006_cycle064.cal")
mtd.deleteWorkspace("aligned")
FindPeaks("focussed","smootheddata","peakslist")

peaks = mtd.getTableWorkspace("peakslist")

print "Number of Columns: " + str(peaks.columnCount()) + " , Number of rows: "+ str(peaks.rowCount())

colNames = peaks.getColumnNames()
for i in colNames:
	print i

for i in peaks.getIntColumn("spectrum"):
	print i
	
for i in peaks.getDoubleColumn("centre"):
	print i
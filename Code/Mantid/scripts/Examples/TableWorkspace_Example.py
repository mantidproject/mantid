# Perform some algorithms
LoadRaw(Filename="GEM40979.raw",OutputWorkspace="GEM40979")
AlignDetectors("GEM40979",OutputWorkspace="aligned",CalibrationFile="offsets_2006_cycle064.cal")
DeleteWorkspace("GEM40979")
DiffractionFocussing("aligned","offsets_2006_cycle064.cal",OutputWorkspace="focussed")
DeleteWorkspace("aligned")
FindPeaks("focussed",PeaksList="peakslist")

peaks = mtd["peakslist"]
help(peaks)
print "Number of Columns: " + str(peaks.columnCount()) + " , Number of rows: "+ str(peaks.rowCount())

colNames = peaks.getColumnNames()
for i in colNames:
    print i

for i in range(peaks.rowCount()):
    row = peaks.row(i)
    print "Spectrum %d has peak at pos %f" % (row['spectrum'], row['centre'])


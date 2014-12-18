##-------------------------------------------------------------------------
##
## Example: Calculating the solid angle for a spectrum with respect to the
##          sample position
##
##-------------------------------------------------------------------------
# Prompt for a raw file to load
rawWSTitle = "RawFile"
LoadRawDialog(OutputWorkspace=rawWSTitle)

# Get the loaded workspace from Mantid
rawData = mantid.getMatrixWorkspace(rawWSTitle)
# Find the position of the sample and beamline. This is a 3D vector with X(), Y() and Z() functions
sample = rawData.getInstrument().getSample()
samplePos = sample.getPos()
beamPos = samplePos - rawData.getInstrument().getSource().getPos()

# Pick a workspace index
wsIndex = 0
# Find the detector associated with the spectrum location at position wsIndex
detector = rawData.getDetector(0)

# Find r, theta and phi
r = detector.getDistance(sample)
twoTheta = detector.getTwoTheta(samplePos, beamPos)*180./math.pi
phi = detector.getPhi()

print 'R = ' + str(r) + ', TwoTheta = ' + str(twoTheta)+ ', Phi = ' + str(phi)

# Check if the detector is masked out and calculate the result if not
solidAngle = 0.0
if not detector.isMasked():
    sAngle = detector.solidAngle(samplePos)

print "The solid angle of the spectrum located at index " + str(wsIndex) + " is: " + str(sAngle) + " steradians"


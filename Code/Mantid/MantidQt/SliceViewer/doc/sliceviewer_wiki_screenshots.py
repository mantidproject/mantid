""" Generate screenshots for the wiki docs
"""

# Basic parameters
filename = "TOPAZ_3131_event.nxs"
ws = "TOPAZ"
LoadEventNexus(Filename=filename,OutputWorkspace=ws+"_nxs")
ConvertToDiffractionMDWorkspace(InputWorkspace=ws+"_nxs",OutputWorkspace=ws, LorentzCorrection='1', SplitInto='2',SplitThreshold='150')
FindPeaksMD(InputWorkspace=ws,MaxPeaks='500',OutputWorkspace=ws+'_peaks')
FindUBUsingLatticeParameters(PeaksWorkspace=ws+'_peaks',a='10.3522',b='6.0768',c='4.7276', alpha='90',beta='90',gamma='90', NumInitial='20', Tolerance='0.12')
IndexPeaks(PeaksWorkspace=ws+'_peaks', Tolerance='0.12')
CopySample(InputWorkspace=ws+'_peaks',OutputWorkspace=ws+"_nxs", CopyName='0',CopyMaterial='0',CopyEnvironment='0',CopyShape='0')
ConvertToDiffractionMDWorkspace(InputWorkspace=ws+"_nxs", OutputWorkspace='HKL', OutputDimensions='HKL',LorentzCorrection='1', SplitInto='2',SplitThreshold='150')

#================ Start the Screenshots ==========================
import numpy
from PyQt4 import Qt

svw = plotSlice("hkl", slicepoint=[0,0, -5], colorscalelog=True, limits=[-6.5,-3.5, -2.5,0.5])
svw.setColorScaleAutoSlice()

n = 0
for L in numpy.arange(-5.07, -4.9, 0.01):
	svw.setSlicePoint(2, L)
	Qt.QApplication.processEvents()
	pix = Qt.QPixmap.grabWidget(svw._getHeldObject())
	pix.save("/home/8oz/Code/Mantid/Code/Mantid/MantidQt/SliceViewer/doc/anim%02d.png" % n)
	n = n + 1
	
import os
# This requires imagemagick. Converts to a nifty animated gif.
os.system("convert /home/8oz/Code/Mantid/Code/Mantid/MantidQt/SliceViewer/doc/anim*.png /home/8oz/Code/Mantid/Code/Mantid/MantidQt/SliceViewer/doc/SliceViewer_SlicePoint_Animation.gif")

# =============
BinMD(InputWorkspace='topaz_q',AlignedDimX='Q_lab_x, 0, 6, 120',AlignedDimY='Q_lab_y, -3, 3, 120',AlignedDimZ='Q_lab_z, 0, 6, 120', OutputWorkspace='bin_q')

sv = plotSlice('bin_q', slicepoint=[0,0, 4.15], colorscalelog=True, limits=[2,4,-1,1])
n = 0
for y in np.linspace(-0.2, 0.2, 21):
	lv = sv.showLine(start=[2.5,y], end=[3.5,y], width=0.1)
	n += 1
	pix = QtGui.QPixmap.grabWidget(sv._getHeldObject())
	pix.save("anim%02d.png" % n)
	

closeAllSliceViewers()
#!/usr/bin/python
################################################################################
# Customize the widigets in a gui python file generated from pyuic4
################################################################################
import sys
import shutil
import datetime

def main(argv):
    """ Main
    """
    if len(argv) < 2:
        print "Input: %s [pyqt python file name]" % (argv[0])
        return

    # import 
    pfilename = argv[1]
    if pfilename.endswith('.') is True:
        pfilename += "py"
    try:
        pfile = open(pfilename, 'r')
        lines = pfile.readlines()
        pfile.close()
    except IOError as e:
        raise e

    # move the source file
    shutil.move(pfilename, pfilename+".bak")

    # replace and add import
    wbuf = ""
    importclass = True
    for line in lines:
        if line.count('class') == 1 and line.count('):') == 1 and importclass is True:
            # add import
            wbuf += 'from MplFigureCanvas import *\n'
            importclass = False
        if line.count('QtGui.QGraphicsView(') == 1:
            # replace QGraphicsView by Qt4MplCanvas
            line = line.replace('QtGui.QGraphicsView(', 'Qt4MplPlotView(')

        wbuf += line

    # write to file
    ofile = open(pfilename, 'w')
    ofile.write(wbuf)
    ofile.close()

    return

if __name__ == "__main__":
    main(sys.argv)

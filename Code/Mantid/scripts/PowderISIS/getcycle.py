import sys
import os.path


def rawpath(runno, inst='hrp', Verbose=False):
    ofile = open('//filepath.isis.rl.ac.uk/' + inst + str(runno) + '.raw/windir.txt', 'r')
    line = os.path.abspath(ofile.readline()) + '\\' + inst + str(runno) + '.raw'
    if Verbose:
        print '//filepath.isis.rl.ac.uk/' + inst + str(runno) + '.raw/windir.txt'
        print line
    return line

# __main__
if __name__ == '__main__':
    rawpath(sys.argv[1], Verbose=True)


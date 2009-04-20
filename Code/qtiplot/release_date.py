import sys
from datetime import date

# argv[1] can optionally be a version number
# argv[2] can optionally be a subversion revision

def main(argv=None) :
    if argv is None :
        argv = sys.argv
    f = open('qtiplot/src/Mantid/MantidPlotReleaseDate.h','w')
    f.write('#ifndef MANTIDPLOT_RELEASE_DATE\n')
    f.write('#define MANTIDPLOT_RELEASE_DATE "')
    f.write(date.today().strftime("%d %b %Y"))
    if len(sys.argv) > 1 :
        f.write(' (Version '+argv[1])
        if len(sys.argv) > 2 :
            f.write(', SVN R'+argv[2])
        f.write(')')
    f.write('"\n#endif\n')
    f.close()
    return 0

if __name__ == "__main__" :
    sys.exit(main())

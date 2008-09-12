from datetime import date
f = open('qtiplot/src/Mantid/MantidPlotReleaseDate.h','w')
f.write('#ifndef MANTIDPLOT_RELEASE_DATE\n')
f.write('#define MANTIDPLOT_RELEASE_DATE "')
f.write(date.today().strftime("%d %b %Y"))
f.write('"\n#endif\n')
f.close()

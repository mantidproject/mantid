#-------------------------------------------------------------------
# mantidplot.py
# 
# Load 'pymantidplot' which cannot be called 'mantidplot' because of 
# name conflict with the MantidPlot binary (especially on osx)
#-------------------------------------------------------------------

try:
    import pymantidplot
except ImportError:
    raise ImportError('Could not import mantidplot (when trying to import pymantidplot). Something is broken in this installation, please check.')

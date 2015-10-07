#-------------------------------------------------------------------
# mantidplot.py
# 
# Load 'pymantidplot' which cannot be called 'mantidplot' because of 
# name conflict with the MantidPlot binary (especially on osx)
#-------------------------------------------------------------------

try:
    import pymantidplot
    from pymantidplot import *
except ImportError:
    raise ImportError('Could not import mantidplot (when trying to import pymantidplot). Something is broken in this installation, please check.')

try:
    # import pyplot and also bring it into the standard MantidPlot namespace
    import pymantidplot.pyplot
    from pymantidplot.pyplot import *
except ImportError:
    raise ImportError('Could not import pymantidplot.pyplot. Something is broken in this installation, please check.')

try:
    import pymantidplot.qtiplot
except ImportError:
    raise ImportError('Could not import pymantidplot.qtiplot. Something is broken in this installation, please check.')

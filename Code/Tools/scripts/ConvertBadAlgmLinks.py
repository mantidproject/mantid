import re, glob, os

def grep(patt,lines):
    """ finds patt in file - patt is a compiled regex
        returns all lines that match patt """
    matchlines = []
    for line in lines:
        match = patt.search(line)
        if match:
            matchline = match.group()
            matchlines.append(matchline)
    results = '\n '.join(matchlines)
    if results:
        return results
    else:
        return None

#get alg names
algs = AlgorithmFactory.getRegisteredAlgorithms(True)
#algs = ['Abragam','BackToBackExponential','BivariateNormal','BSpline','Chebyshev','ChudleyElliot','CompositeFunction','Convolution','CubicSpline','DiffRotDiscreteCircle','DiffSphere','DSFInterp1DFit','ExpDecay','ExpDecayMuon','ExpDecayOsc','FickDiffusion','FlatBackground','GausDecay','GausOsc','Gaussian','HallRoss','IkedaCarpenterPV','LatticeErrors','LinearBackground','LogNormal','Lorentzian','MuonFInteraction','NeutronBk2BkExpConvPVoigt','PeakHKLErrors','ProductFunction','ProductLinearExp','ProductQuadraticExp','Quadratic','SCDPanelErrors','StaticKuboToyabe','StaticKuboToyabeTimesExpDecay','StaticKuboToyabeTimesGausDecay','StretchedExpFT','StretchExp','StretchExpMuon','TeixeiraWater','ThermalNeutronBk2BkExpConvPVoigt','UserFunction','Voigt']
regexs= {}
for alg in algs:
    regexs[alg] = re.compile(r'`%s\s+<[\w\:\/\.]+\/%s>`_' % (alg,alg))


# Example use
dir = r"C:\Mantid\Code\Mantid\docs\source\algorithms"
files = glob.glob(os.path.join(dir, '*.rst'))
for filename in files:

    #print os.path.basename(filename)[:-4]
    with open(filename) as file:
      lines = file.readlines()
      for alg in algs:
        expr = regexs[alg]
        results = grep(expr, lines)
        if results:
          print filename
          print results




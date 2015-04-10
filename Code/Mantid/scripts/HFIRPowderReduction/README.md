Use cases for tabs: 

  1. **Raw Detectors**: Visualize the reading of detectors directly coming out of the raw data
    * Plot N lines for N Pts.;
    * Highlight (make it thicker) the Pt that is interested;
    * New from Mantid:  *ReadRawSpiceSignal(Pts)*;
  2. **Individual Detector**: Visual the readings of one detector across an experiment
    * Plot the counts of any individual detector;
    * Able to change the X-axis from 2theta to arbitrary sample environment log;
    * New from Mantid: *ReadRawSpiceSignal(DetectorID, XLabel)*;
  3. **Normalized**: Reduce one scan each time
    * Plot the reduced data
    * Automatically locate detector efficiency file
    * New from Mantid: *ConvertCWPDMDToSpectra(ExcludedDetectors=[])*
    * New from Mantid: *ConvertSpiceDataToRealSpace(DetectorEfficiencyTable)*
  4. **Multiple Scans**: Reduce a set of scans
    * Reduce a set of scans and plot in 2D/water-fall mode;
    * Able to merge all the scans;
    * New from Mantid: *ConvertCWPDMDToSpectra(ExcludedDetectors=[])*
  5. **Vanadium**: strip vanadium peaks
    * Strip vanadium peak with unit 'Degrees' because of the binning (range and step size) must be respected;
    * Peaks' position should be calculated and indicated auotmatically;
    * *Mantid::StripPeaks()* will be called instread of *StripVadadiumPeaks()* because
      the later one only works in d-spacing;
  6. **Advanced Setup**
    * URL for raw data files;



Tasks:
  
  1. Make tab *Raw Detector* work;
  2. Make tab *Individual Detector* work;
  3. Find out why the vanadium runs (379-10/11) look funny, i.e., oscilating curves;
  4. Implement automatic wavelength mapping; 
  5. Implement tab *vanadium*;
  6. Talk with Clarina how to deal with vanadium spectrum with peaks striped; 

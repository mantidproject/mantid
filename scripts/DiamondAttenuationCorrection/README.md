This implements a Diamond Attenuation Correction for performing
neautron TOF measurements in Diamond Anvil Cells.

It contains a functionality to generate the UB matrices for the 2
diamonds from a peaks input file where one has selected diamond peaks
in Mantid beforehand.

Uses that and the normalised, focused and background-corrected
transmission data to extract the diamond attenuation and outputs it
alongside the data with the attenuation excluded.

This output can then be focused and the results will be a refinable
pattern.

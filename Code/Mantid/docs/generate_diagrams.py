#!/usr/bin/env python

######################
#CONFIGURABLE OPTIONS#
######################

STYLE = dict()

STYLE["global_style"] = """
fontname = Helvetica
labelloc = t
node[fontname="Helvetica", style = filled]
edge[fontname="Helvetica"]
"""

STYLE['param_style']     = 'node[fillcolor = khaki, shape = oval]'
STYLE['decision_style']  = 'node[fillcolor = limegreen, shape = diamond]'
STYLE['algorithm_style'] = 'node[style = "rounded,filled", fillcolor = lightskyblue, shape = rectangle]'
STYLE['process_style']   = 'node[fillcolor = lightseagreen, shape = rectangle]'
STYLE['value_style']     = 'node[fontname = "Times-Roman", fillcolor = grey, shape = parallelogram]'

#############################
#END OF CONFIGURABLE OPTIONS#
#############################

import argparse, os, string, subprocess

parser = argparse.ArgumentParser(description="Generate diagrams from dot files")

parser.add_argument("output_dir", help="The directory to write the output files to")
parser.add_argument("input_files", nargs="+", help="The .dot files to process")

args = parser.parse_args()

print("Output dir: " + args.output_dir)

if not os.path.exists(args.output_dir):
    os.makedirs(args.output_dir)

for fn in args.input_files:
    if fn.endswith(".dot"):
        dot_name = os.path.basename(fn)
        png_name = dot_name[:-4] + ".png"
        png_path = os.path.join(args.output_dir, png_name)

        print("Reading: " + fn)

        in_src = open(fn, 'r').read()
        out_src = string.Template(in_src).substitute(STYLE)

        print("Writing: " + png_path)
        gviz = subprocess.Popen(["dot","-Tpng","-o",png_path], stdin=subprocess.PIPE)
        gviz.communicate(input=out_src)
        gviz.wait()


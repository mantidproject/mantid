from base import BaseDirective
from sphinx.locale import _
import os, string, subprocess

######################
#CONFIGURABLE OPTIONS#
######################

STYLE = dict()

STYLE["global_style"] = """
fontname = Helvetica
labelloc = t
ordering = out
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


class DiagramDirective(BaseDirective):

    """
    Adds a diagram from a dot source file

    It requires DIAGRAMS_DIR and DOT_EXECUTABLE environment variables to be set to the
    directory where a diagram should be generated and where the dot program may be found,
    respectively. If they are not set then a RuntimeError occurs
    """

    required_arguments, optional_arguments = 1, 0

    def run(self):
        """
        The main entry point that docutils calls.
        It calls self.execute to do the main work.
        Derived classes should override execute() and insert
        whatever rst they require with self.add_rst()
        """
        nodes = self.execute()
        if self.rst_lines is not None:
            self.commit_rst()
        return nodes

    def execute(self):
        env = self.state.document.settings.env

        try:
            out_dir = os.environ["DIAGRAMS_DIR"]
        except:
            raise RuntimeError("The '.. diagram::' directive requires a DIAGRAMS_DIR environment variable to be set.")

        try:
            dot_executable = os.environ["DOT_EXECUTABLE"]
        except:
            self.add_rst(".. figure:: /images/ImageNotFound.png\n\n" +
                        "    graphviz not found - diagram could not be rendered.")
            return []

        #Make sure we have an output directory
        if not os.path.exists(out_dir):
            os.makedirs(out_dir)

        diagram_name = self.arguments[0]
        if diagram_name[-4:] != ".dot":
            raise RuntimeError("Diagrams need to be referred to by their filename, including '.dot' extension.")

        in_path = os.path.join(env.srcdir, "diagrams", diagram_name)
        out_path = os.path.join(out_dir, diagram_name[:-4] + ".png")

        #Generate the diagram
        try:
            in_src = open(in_path, 'r').read()
        except:
            raise RuntimeError("Cannot find dot-file: '" + diagram_name + "' in '" + os.path.join(env.srcdir,"diagrams"))

        out_src = string.Template(in_src).substitute(STYLE)
        gviz = subprocess.Popen([dot_executable,"-Tpng","-o",out_path], stdin=subprocess.PIPE)
        gviz.communicate(input=out_src)
        gviz.wait()

        #relative path to image, in unix style
        rel_path = os.path.relpath(out_path, env.srcdir).replace("\\","/")

        self.add_rst(".. image:: /" + rel_path + "\n\n")

        return []

#------------------------------------------------------------------------------------------------------------

def setup(app):
    """
    Setup the directives when the extension is activated

    Args:
      app: The main Sphinx application object
    """
    app.add_directive('diagram', DiagramDirective)

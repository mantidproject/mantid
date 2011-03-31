import sys

if len(sys.argv)<2:
    print "Usage: python generate_stubs.py [NAME_OF_INSTRUMENT]"
print "Generating stubs for: %s\n" % sys.argv[1]

instr_lowercase = sys.argv[1].lower().strip()
instr_name = sys.argv[1]

def _replace_strings(f_in, f_out):
    f = open(f_in, 'r')
    output = open(f_out, 'w')
    for line in f.readlines():
        line = line.replace("%INSTR_NAME_LOWERCASE%", instr_lowercase)
        line = line.replace("%INSTR_NAME%", instr_name)
        output.write(line)
    output.close()
    print "File %s created" % f_out

# Interface class
_replace_strings("interface_template.py", "../reduction_gui/instruments/%s_interface.py" % instr_lowercase)
# Scripter class
_replace_strings("scripter_template.py", "../reduction_gui/reduction/%s_scripter.py" % instr_lowercase)
# Script elements
_replace_strings("script_elements_template.py", "../reduction_gui/reduction/%s_script_elements.py" % instr_lowercase)
# Script element widgets
_replace_strings("script_elements_widget_template.py", "../reduction_gui/widgets/%s_script_widget.py" % instr_lowercase)


print "\nDon't forget to add your instrument to instrument_factory.py!"

print "Add the following line at the top of the file:"
print
print "from %s_interface import %sInterface" % (instr_lowercase, instr_name)
print
print "Then add the following line after the definition of INSTRUMENT_DICT:"
print 
print "INSTRUMENT_DICT['%s'] = %sInterface" % (instr_name, instr_name)

    


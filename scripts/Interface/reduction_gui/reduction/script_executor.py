try:
    import mantidplot
    HAS_MANTIDPLOT = True
    HAS_MANTID_QT = False
except(ImportError, ImportWarning):
    HAS_MANTIDPLOT = False
    try:
        from mantidqt.widgets.codeeditor.execution import PythonCodeExecution
        HAS_MANTID_QT = True
    except(ImportError, ImportWarning):
        HAS_MANTID_QT = False

import traceback


def __get_indent(line):
    return len(line) - len(line.lstrip(' \t'))


def splitCodeString(string, return_compiled=True):
    import code
    lines = string.splitlines(False)
    lines.append("\n") # append empty line, so the last staement always gets executed.

    statement_start = 0
    line_nbr = 0
    current_statement = ''
    base_indent = 0 # indentation of first line
    try:
        for line in lines:
            line_nbr += 1

            if __get_indent(line) > base_indent:
                # if inident of this line is bigger than base indent,
                # previous lines cannot be a full statment, so append this line:
                current_statement += line + '\n'
                continue
            else:
                # previous lines might be a full statment...
                code_object = code.compile_command(current_statement, filename="<input>", symbol="single")
                if code_object is not None:
                    if current_statement.strip():
                        yield (code_object if return_compiled else current_statement, (statement_start, line_nbr))
                    statement_start = line_nbr
                    current_statement = '\n' * line_nbr
                # previousn lines aren't a full statement
                # or they habe been returned and current_statement has been cleared
                # so append the new line:
                current_statement += line + '\n'
                continue

    except SyntaxError as e:
        e.lineno += statement_start
        print(str(e))
        print(e.text + ' ' * (e.offset-1) + '^')
        print('~~~~~~~~~~~~~~~~~~~~~~~~')
        print(current_statement)
        raise e
    except OverflowError as e:
        raise e
    except ValueError as e:
        raise e


def __wrappedExec(code, globals_dict, locals_dict):
    # wrap exec here, because python doesnt like exec in nested functions
    exec(code, globals_dict, locals_dict)
    return True


def execute_script(script, progress_action):
    """
        @param script: the script to execute
        @param progress_action: a callable expresion that takes two arguments:
            - progress: a value between 0 and 1 indicating the current progress
            - state: a string indicating the current state ('running', 'success' or 'failure')
    """
    if HAS_MANTIDPLOT:
        def execFunc(code):
            return mantidplot.runPythonScript(code, async=True)
    elif HAS_MANTID_QT:
        codeExecuter = PythonCodeExecution()

        def execFunc(code):
            codeExecuter.execute_async(code)
            return True
    else:
        # exec doesn't keep track of globals and locals, so we have to do it manually:
        globals_dict = dict()
        locals_dict = dict()

        def execFunc(code):
            return __wrappedExec(code, globals_dict, locals_dict)

    code_blocks = [code for code in splitCodeString(script, return_compiled = not HAS_MANTIDPLOT)]
    code_blocks.insert(0, ('from mantid.simpleapi import *\n', (0,)))

    def progress(i, state):
        progress_action(float(i) / float(len(code_blocks)), state)

    for i, (code, position) in enumerate(code_blocks):
        try:
            progress(i, 'running')
            if not execFunc(code):
                return progress(i, 'failure')
        except Exception as e:
            traceback.print_exc()
            progress(i, 'failure')
            raise e
    progress_action(1.0, 'success')

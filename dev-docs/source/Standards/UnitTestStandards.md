# Unit Test Coding Standards

The code for unit test classes should follow the standards as set out in
[Cpp Coding Standards](CppCodingStandards) with the following differences.

- The file and class name should be of the form [Class Name Test](ClassNameTest), for
  example [Workspace Test](WorkspaceTest), [Logger Test](LoggerTest)
- Test classes do not need to be included in a namespace.
- Test methods within the test classes must start with the word 'test'
  and then describe the test. For example [Test Read Xvalues](test_readXValues),
  [Test Invalid Log String](test_invalidLogString)
- Other utility methods may be used in test classes but must not start
  with the word test
- During debugging Test methods may be disabled by prefixing the method
  name with 'x'. However if any disabled tests are to be checked in this
  must be accompanied with comments explaining why the test is disabled
  and when it may be enabled again.
- Code headers are not required if the above standards are met as the
  use of the class will be obvious from the class name and method names.

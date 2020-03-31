module OuterModule {
  // This test and check_symbol_accesses* test that the variable "a"
  // can only be accessed by things that "use outermost;".
  // check_symbol_accesses checks a legal access and 3 checks another illegal.
  module outermost {
    var a = 3;
    module middlemost {
      module innermost {
        proc foo() {
          import OuterModule.outermost.a;
          a = 4;
        }
      }
    }
  }
  proc main() {
    use outermost.middlemost.innermost;
    bar(); foo(); bar();
  }

  proc bar() {
    writeln(a);
  }
}

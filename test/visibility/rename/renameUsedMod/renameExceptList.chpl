module Long {
  var x: int = 2;
  var y: bool = false;
}
module User {
  use Long as L except y;

  proc main() {
    writeln(L.x);
    writeln(L.y); // Was excluded, so should only be findable with explicit name
  }
}

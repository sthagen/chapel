var a = __primitive("get compiler variable", "CHPL_HOME");
var b = __primitive("get compiler variable", "chpl_lookupFilename", "CHPL_HOME");
var c = __primitive("get compiler variable", a, b);
var d = __primitive("get compiler variable", "chpl_lookupFilename", CHPL_HOME);
var e = if true then 1 else 2;
var f = for i in 1..10 do i;
var g = forall i in 1..10 do i;
var h = [i in 1..10] i;
var i = try! x;
var j = try x;
var k = foo("this is", a, "function", call);

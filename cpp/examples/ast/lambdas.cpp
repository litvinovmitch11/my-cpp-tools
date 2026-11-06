int applyOffset(int Value) {
  const int Offset = 3;
  const auto AddOffset = [Offset](int Input) { return Input + Offset; };
  return AddOffset(Value);
}

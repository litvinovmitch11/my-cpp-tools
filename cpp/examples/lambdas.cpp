int applyOffset(int Value) {
  const int Offset = 3;
  const auto AddOffset = [Offset](int Input) { return Input + Offset; };
  return AddOffset(Value);
}

int apply(int Value) {
  auto Transform = [Value](int Argument) {
    if (Argument > Value)
      return Argument;
    return Value;
  };
  return Transform(Value);
}

int makeCapture();

int captureInitializer() {
  auto Function = [Value = makeCapture()] { return Value; };
  return Function();
}

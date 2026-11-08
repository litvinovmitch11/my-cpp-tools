int parse(int Value);

int parseOrDefault(int Value) {
  try {
    if (Value < 0)
      throw Value;
    return parse(Value);
  } catch (int) {
    return 0;
  }
}

int normalize(int Value) {
  if (Value < 0)
    goto Reset;
  return Value;

Reset:
  return 0;
}

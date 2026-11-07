int parseOrDefault(int Value) {
  try {
    if (Value < 0)
      throw Value;
    return Value;
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

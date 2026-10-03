export const INITIAL_CODE = `int f(int x) {
  if (x > 2)
    return (x / 42);
  return x + 1;
}

int g() {
  int b = 0;
  for (int i = 0; i < 123; ++i) {
    b += 3 * i + 2;
  }
  return b;
}
`;

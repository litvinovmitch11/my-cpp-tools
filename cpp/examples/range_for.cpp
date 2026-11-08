int sumValues(const int (&Values)[3]) {
  int Result = 0;
  for (const int Value : Values)
    Result += Value;
  return Result;
}

struct Iterator {
  int operator*() const;
  Iterator &operator++();
  bool operator!=(const Iterator &) const;
};

struct ThrowingRange {
  Iterator begin();
  Iterator end();
};

int sumThrowingRange(ThrowingRange &Range) {
  int Result = 0;
  try {
    for (const int Value : Range)
      Result += Value;
  } catch (...) {
    return -1;
  }
  return Result;
}

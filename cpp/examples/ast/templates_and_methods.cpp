template <typename ValueType>
ValueType maximum(ValueType Left, ValueType Right) {
  if constexpr (sizeof(ValueType) > 1)
    return Left < Right ? Right : Left;
  return Left;
}

struct Counter {
  int Value;

  int increment(int Amount) {
    Value += Amount;
    return Value;
  }
};

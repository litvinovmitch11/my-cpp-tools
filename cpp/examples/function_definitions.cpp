template <typename ValueType> ValueType identity(ValueType Value) {
  return Value;
}

template <> int identity<int>(int Value) { return Value + 1; }

struct Number {
  Number() = default;
  Number(const Number &Other) = delete;
  ~Number() = default;

  int operator()(int Value) const { return identity(Value); }
};

long instantiateIdentity(long Value) { return identity(Value); }

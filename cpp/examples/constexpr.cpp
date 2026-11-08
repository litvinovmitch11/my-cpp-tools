constexpr int concrete() {
  if constexpr (true)
    return 1;
  else
    return 2;
}

constexpr int discardedThrowingBranch() {
  if constexpr (true)
    return 1;
  else
    throw 2;
}

template <typename Value> int dependent(Value Input) {
  if constexpr (sizeof(Value) > 4)
    return Input + 1;
  else
    return Input - 1;
}

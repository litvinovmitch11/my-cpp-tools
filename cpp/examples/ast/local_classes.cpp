int useLocalClass(int Value) {
  class Multiplier {
  public:
    explicit Multiplier(int Factor) : Factor(Factor) {}

    int apply(int Input) const { return Input * Factor; }

  private:
    int Factor;
  };

  const Multiplier Double(2);
  return Double.apply(Value);
}

int initializeFactor(int Factor);

int useLocalClass(int Value) {
  class Multiplier {
  public:
    explicit Multiplier(int Factor) try : Factor(initializeFactor(Factor)) {
    } catch (...) {
    }

    ~Multiplier() noexcept(false) try {
    } catch (...) {
    }

    int apply(int Input) const { return Input * Factor; }

  private:
    int Factor;
  };

  const Multiplier Double(2);
  return Double.apply(Value);
}

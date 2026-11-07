int combine(int Left, int Right) {
  const int Product = Left * Right;
  const int Magnitude = Product < 0 ? -Product : Product;
  return static_cast<int>(Magnitude + 1.5);
}

bool compare(const char *Text, char Initial) {
  return Text != nullptr && Text[0] == Initial;
}

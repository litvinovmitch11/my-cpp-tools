int extensionControl(int Value) {
  void *Target = &&Done;
  if (Value < 0)
    goto *Target;

  switch (Value) {
  case 1 ... 3:
    return Value;
  default:
    break;
  }

Done:
  asm volatile("" ::: "memory");
  ;
  return 0;
}

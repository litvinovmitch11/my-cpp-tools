int classify(int Value) {
  if (Value < 0)
    return -1;
  if (Value == 0)
    return 0;
  return 1;
}

int accumulate(int Limit) {
  int Result = 0;
  for (int Index = 0; Index < Limit; ++Index) {
    if (Index == 2)
      continue;
    Result += Index;
  }

  while (Result > 20)
    --Result;

  do {
    ++Result;
  } while (Result < 10);

  switch (Result) {
  case 10:
    return Result;
  default:
    break;
  }
  return 0;
}

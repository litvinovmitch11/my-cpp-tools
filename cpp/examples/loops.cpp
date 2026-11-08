int loops(const int (&Values)[3], int Limit) {
  int Result = 0;
  for (int Index = 0; Index < Limit; ++Index) {
    if (Index == 2)
      continue;
    Result += Index;
  }

  while (Result > 20) {
    --Result;
    if (Result == 15)
      break;
  }

  do {
    ++Result;
  } while (Result < 10);

  for (const int Value : Values)
    Result += Value;
  return Result;
}

int sumValues(const int (&Values)[3]) {
  int Result = 0;
  for (const int Value : Values)
    Result += Value;
  return Result;
}

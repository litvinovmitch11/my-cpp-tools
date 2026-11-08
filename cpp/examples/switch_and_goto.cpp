int selectValue();

int route(int Value) {
  int Result = 0;
  switch (Value < 0 ? -Value : Value) {
  case 1:
  case 2:
    Result = 10;
    break;
  case 3:
    Result = 30;
  default:
    Result += 1;
  }

  if (Result < 0)
    goto done;
  Result += 2;

done:
  return Result;
}

int switchCall() {
  switch (selectValue()) {
  case 0:
    return 1;
  default:
    return 2;
  }
}

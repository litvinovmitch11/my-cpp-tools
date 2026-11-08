int unreachable(bool Skip) {
  int Dead = 0;
  if (Skip)
    goto done;

  return 1;
  Dead = 2;

done:
  return Dead;
}

[[noreturn]] void terminateAnalysis();

int afterNoreturnCall() {
  terminateAnalysis();
  return 0;
}

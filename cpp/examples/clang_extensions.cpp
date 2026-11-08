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

int assemblyBranch(bool Jump) {
  if (Jump)
    asm goto("" : : : : Done);
  return 0;

Done:
  return 1;
}

int fallback();

int gnuConditional(int Value) { return Value ?: fallback(); }

int compileTimeChoice() { return __builtin_choose_expr(true, 1, fallback()); }

void compileTimeExpressionStatement() {
  (void)__builtin_choose_expr(false, fallback(), 0);
}

int statementExpression(bool Choose) {
  return ({
    int Result = 0;
    if (Choose)
      Result = 1;
    Result;
  });
}

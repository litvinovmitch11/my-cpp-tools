void consume(bool Value);
int combine(bool Left, bool Right);

int choose(int Left, int Right, bool Ready) {
  if (Ready && (Left < Right || Right == 0))
    return Left;

  int Result = Ready ? Right : Left;
  consume(Ready && Result > 0);
  return Result;
}

int unspecifiedOrder(bool First, bool Second, bool Third, bool Fourth) {
  return combine(First && Second, Third || Fourth);
}

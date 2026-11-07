const char *message(bool Detailed) {
  return Detailed ? "line\n\"quoted\"" : "plain";
}

char pathSeparator() { return '\\'; }

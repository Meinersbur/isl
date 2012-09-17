{
  for (int c0 = a3; c0 <= min(min(a2 - 1, a1 - 1), b3); c0 += 1)
    s2(c0);
  for (int c0 = a1; c0 <= min(a2 - 1, b1); c0 += 1) {
    s0(c0);
    if (c0 >= a3 && b3 >= c0)
      s2(c0);
  }
  for (int c0 = max(max(b1 + 1, a1), a3); c0 <= min(a2 - 1, b3); c0 += 1)
    s2(c0);
  for (int c0 = a2; c0 <= b2; c0 += 1) {
    if (c0 >= a1 && b1 >= c0)
      s0(c0);
    s1(c0);
    if (c0 >= a3 && b3 >= c0)
      s2(c0);
  }
  for (int c0 = max(max(b2 + 1, a3), a2); c0 <= min(a1 - 1, b3); c0 += 1)
    s2(c0);
  for (int c0 = max(max(b2 + 1, a1), a2); c0 <= b1; c0 += 1) {
    s0(c0);
    if (b3 >= c0 && c0 >= a3)
      s2(c0);
  }
  for (int c0 = max(max(max(max(b1 + 1, b2 + 1), a1), a3), a2); c0 <= b3; c0 += 1)
    s2(c0);
}

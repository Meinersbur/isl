{
  for (int c0 = a2, c0_end = min(min(a1 - 1, a3 - 1), b2); c0 <= c0_end; c0 += 1)
    s1(c0);
  for (int c0 = a1, c0_end = min(b1, a3 - 1); c0 <= c0_end; c0 += 1) {
    s0(c0);
    if (c0 >= a2 && b2 >= c0)
      s1(c0);
  }
  for (int c0 = max(max(a1, b1 + 1), a2), c0_end = min(a3 - 1, b2); c0 <= c0_end; c0 += 1)
    s1(c0);
  for (int c0 = a3, c0_end = b3; c0 <= c0_end; c0 += 1) {
    if (c0 >= a1 && b1 >= c0)
      s0(c0);
    if (c0 >= a2 && b2 >= c0)
      s1(c0);
    s2(c0);
  }
  for (int c0 = max(max(a3, b3 + 1), a2), c0_end = min(a1 - 1, b2); c0 <= c0_end; c0 += 1)
    s1(c0);
  for (int c0 = max(max(a1, a3), b3 + 1), c0_end = b1; c0 <= c0_end; c0 += 1) {
    s0(c0);
    if (c0 >= a2 && b2 >= c0)
      s1(c0);
  }
  for (int c0 = max(max(max(max(a1, b1 + 1), a3), b3 + 1), a2), c0_end = b2; c0 <= c0_end; c0 += 1)
    s1(c0);
}

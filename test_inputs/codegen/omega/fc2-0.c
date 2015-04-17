for (int c0 = 0, c0_end = n - 1; c0 < c0_end; c0 += 1) {
  for (int c3 = 0, c3_end = n - c0 - 1; c3 < c3_end; c3 += 1)
    s0(c0 + 1, n - c3);
  for (int c3 = 0, c3_end = n - c0 - 1; c3 < c3_end; c3 += 1)
    for (int c6 = c0 + 2, c6_end = n; c6 <= c6_end; c6 += 1)
      s1(c0 + 1, n - c3, c6);
}

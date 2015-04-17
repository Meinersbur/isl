for (int c0 = 2, c0_end = n; c0 <= c0_end; c0 += 2) {
  if (c0 % 4 == 0)
    S2(c0, c0 / 4);
  S1(c0, c0 / 2);
}

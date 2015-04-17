for (int c0 = 1, c0_end = M; c0 <= c0_end; c0 += 1) {
  for (int c1 = 1, c1_end = c0; c1 < c1_end; c1 += 1)
    S1(c0, c1);
  S1(c0, c0);
  S2(c0, c0);
  for (int c1 = c0 + 1, c1_end = M; c1 <= c1_end; c1 += 1)
    S1(c0, c1);
}

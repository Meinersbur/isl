for (int c0 = 1, c0_end = M; c0 < c0_end; c0 += 1)
  for (int c1 = c0 + 1, c1_end = M; c1 <= c1_end; c1 += 1) {
    for (int c3 = 1, c3_end = c0; c3 < c3_end; c3 += 1)
      S1(c0, c3, c1);
    for (int c3 = c0 + 1, c3_end = M; c3 <= c3_end; c3 += 1)
      S2(c0, c3, c1);
  }

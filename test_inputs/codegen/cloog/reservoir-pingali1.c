for (int c0 = 1, c0_end = M; c0 <= c0_end; c0 += 1)
  for (int c1 = 1, c1_end = 2 * N; c1 < c1_end; c1 += 1) {
    for (int c2 = max(1, -N + c1), c2_end = (c1 + 1) / 2; c2 < c2_end; c2 += 1)
      S1(c0, c1 - c2, c2);
    if ((c1 - 1) % 2 == 0)
      S2(c0, (c1 + 1) / 2);
  }

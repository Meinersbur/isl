if (N >= 1) {
  for (int c1 = 1, c1_end = M; c1 <= c1_end; c1 += 1)
    S1(1, c1);
  for (int c0 = 2, c0_end = N; c0 <= c0_end; c0 += 1) {
    for (int c1 = 1, c1_end = M; c1 <= c1_end; c1 += 1)
      S2(c0 - 1, c1);
    for (int c1 = 1, c1_end = M; c1 <= c1_end; c1 += 1)
      S1(c0, c1);
  }
  for (int c1 = 1, c1_end = M; c1 <= c1_end; c1 += 1)
    S2(N, c1);
}

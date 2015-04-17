for (int c0 = 1, c0_end = N; c0 <= c0_end; c0 += 1)
  for (int c1 = 0, c1_end = min(min(M, c0), N - c0); c1 <= c1_end; c1 += 1)
    S1(c0, c1);

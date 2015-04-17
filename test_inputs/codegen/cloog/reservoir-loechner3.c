for (int c0 = 1, c0_end = M; c0 <= c0_end; c0 += 1)
  for (int c1 = 2, c1_end = M + c0; c1 <= c1_end; c1 += 1)
    for (int c2 = max(1, -c0 + c1), c2_end = min(M, c1 - 1); c2 <= c2_end; c2 += 1)
      S1(c0, c2, c1 - c2);

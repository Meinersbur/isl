for (int c0 = 2, c0_end = 2 * M; c0 <= c0_end; c0 += 1)
  for (int c1 = 1, c1_end = M; c1 <= c1_end; c1 += 1)
    for (int c2 = 1, c2_end = M; c2 <= c2_end; c2 += 1)
      for (int c3 = max(1, -M + c0), c3_end = min(M, c0 - 1); c3 <= c3_end; c3 += 1)
        S1(c2, c1, c3, c0 - c3);

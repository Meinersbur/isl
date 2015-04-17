for (int c0 = 1, c0_end = n; c0 < c0_end; c0 += 64)
  for (int c1 = c0 - 1, c1_end = n; c1 <= c1_end; c1 += 64) {
    for (int c2 = c0, c2_end = min(n, c0 + 63); c2 <= c2_end; c2 += 1) {
      for (int c3 = c0, c3_end = min(c1 + 62, c2 - 1); c3 <= c3_end; c3 += 1)
        for (int c4 = max(c1, c3 + 1), c4_end = min(n, c1 + 63); c4 <= c4_end; c4 += 1)
          s1(c3, c4, c2);
      for (int c4 = max(c1, c2 + 1), c4_end = min(n, c1 + 63); c4 <= c4_end; c4 += 1)
        s0(c2, c4);
    }
    for (int c2 = c0 + 64, c2_end = n; c2 <= c2_end; c2 += 1)
      for (int c3 = c0, c3_end = min(c0 + 63, c1 + 62); c3 <= c3_end; c3 += 1)
        for (int c4 = max(c1, c3 + 1), c4_end = min(n, c1 + 63); c4 <= c4_end; c4 += 1)
          s1(c3, c4, c2);
  }

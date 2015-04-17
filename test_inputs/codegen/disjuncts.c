for (int c0 = 0, c0_end = n; c0 <= c0_end; c0 += 1)
  for (int c1 = 0, c1_end = n; c1 <= c1_end; c1 += 1)
    if (c1 == n || c0 == n || c1 == 0 || c0 == 0) {
      for (int c3 = 0, c3_end = n; c3 <= c3_end; c3 += 1)
        for (int c4 = 0, c4_end = n; c4 <= c4_end; c4 += 1)
          a(c0, c1, c3, c4);
      for (int c3 = 0, c3_end = n; c3 <= c3_end; c3 += 1)
        for (int c4 = 0, c4_end = n; c4 <= c4_end; c4 += 1)
          b(c0, c1, c3, c4);
    }

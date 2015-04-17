for (int c0 = 0, c0_end = n - 1; c0 < c0_end; c0 += 32)
  for (int c1 = c0, c1_end = n; c1 < c1_end; c1 += 32)
    for (int c2 = c0, c2_end = n; c2 < c2_end; c2 += 32) {
      if (c1 >= c0 + 32) {
        for (int c3 = c0, c3_end = min(c0 + 31, c2 + 30); c3 <= c3_end; c3 += 1)
          for (int c4 = c1, c4_end = min(n - 1, c1 + 31); c4 <= c4_end; c4 += 1)
            for (int c5 = max(c2, c3 + 1), c5_end = min(n - 1, c2 + 31); c5 <= c5_end; c5 += 1)
              S_6(c3, c4, c5);
      } else
        for (int c3 = c0, c3_end = min(min(n - 2, c0 + 31), c2 + 30); c3 <= c3_end; c3 += 1) {
          for (int c5 = max(c2, c3 + 1), c5_end = min(n - 1, c2 + 31); c5 <= c5_end; c5 += 1)
            S_2(c3, c5);
          for (int c4 = c3 + 1, c4_end = min(n - 1, c0 + 31); c4 <= c4_end; c4 += 1)
            for (int c5 = max(c2, c3 + 1), c5_end = min(n - 1, c2 + 31); c5 <= c5_end; c5 += 1)
              S_6(c3, c4, c5);
        }
    }

for (int c0 = 1, c0_end = 100; c0 <= c0_end; c0 += 1)
  for (int c1 = 1, c1_end = 100; c1 <= c1_end; c1 += 1)
    for (int c2 = 1, c2_end = 100; c2 <= c2_end; c2 += 1) {
      if (c0 >= 61) {
        for (int c3 = 1, c3_end = 100; c3 <= c3_end; c3 += 1)
          for (int c4 = 1, c4_end = 100; c4 <= c4_end; c4 += 1)
            s1(c0, c1, c2, c3, c4);
      } else
        for (int c3 = 1, c3_end = 100; c3 <= c3_end; c3 += 1)
          for (int c4 = 1, c4_end = 100; c4 <= c4_end; c4 += 1) {
            s1(c0, c1, c2, c3, c4);
            s0(c0, c1, c2, c3, c4);
          }
    }

for (int c0 = 1, c0_end = morb; c0 <= c0_end; c0 += 1)
  for (int c1 = 1, c1_end = np; c1 <= c1_end; c1 += 1)
    for (int c2 = 1, c2_end = np; c2 <= c2_end; c2 += 1) {
      if (c2 >= c1)
        s0(c2, c1, c0);
      if (c1 >= c2)
        s1(c1, c2, c0);
    }

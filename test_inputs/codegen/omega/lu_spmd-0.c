if (ub >= lb)
  for (int c0 = 1, c0_end = ub; c0 <= c0_end; c0 += 1)
    for (int c1 = c0, c1_end = n; c1 <= c1_end; c1 += 1) {
      if (c0 >= lb && c1 >= c0 + 1) {
        s0(c0, c1);
        if (n >= ub + 1)
          s2(c0, c1);
      } else if (lb >= c0 + 1)
        s3(c0, c1, lb, c0, c1);
      for (int c3 = max(lb, c0), c3_end = ub; c3 <= c3_end; c3 += 1)
        s1(c0, c1, c3);
    }

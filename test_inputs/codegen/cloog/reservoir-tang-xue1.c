for (int c0 = 0, c0_end = 9; c0 <= c0_end; c0 += 2)
  for (int c1 = 0, c1_end = min(4, c0 + 3); c1 <= c1_end; c1 += 2)
    for (int c2 = max(1, c0), c2_end = min(c0 + 1, c0 - c1 + 4); c2 <= c2_end; c2 += 1)
      for (int c3 = max(1, -c0 + c1 + c2), c3_end = min(4, -c0 + c1 + c2 + 1); c3 <= c3_end; c3 += 1)
        S1(c0 / 2, (-c0 + c1) / 2, -c0 + c2, -c1 + c3);

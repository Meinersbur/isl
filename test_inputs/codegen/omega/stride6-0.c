for (int c0 = 1, c0_end = 101; c0 <= c0_end; c0 += 1)
  for (int c1 = -((c0 - 1) % 2) + c0 + 1, c1_end = 400; c1 <= c1_end; c1 += 2)
    s0(c0, c1);

for (int c0 = 1, c0_end = 5; c0 <= c0_end; c0 += 1)
  for (int c1 = t1 - 64 * b + 64, c1_end = min(70, -((c0 - 1) % 2) - c0 + 73); c1 <= c1_end; c1 += 64)
    A(c0, 64 * b + c1 - 8);

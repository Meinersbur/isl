for (int c0 = 1, c0_end = min(n, 2 * b - 1); c0 <= c0_end; c0 += 1)
  for (int c1 = -b + 1, c1_end = b - c0; c1 <= c1_end; c1 += 1)
    for (int c2 = max(1, c0 + c1), c2_end = min(n, n + c1); c2 <= c2_end; c2 += 1)
      s0(-c0 - c1 + c2 + 1, -c1 + c2, c2);

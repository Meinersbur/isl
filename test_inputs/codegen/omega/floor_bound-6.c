if (m >= 8 * floord(m + 1, 8))
  for (int c0 = 4 * floord(m + 1, 32), c0_end = n; c0 <= c0_end; c0 += 1)
    s0(c0);

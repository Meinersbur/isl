if (m >= 8 * floord(m + 1, 8))
  for (int c0 = 4 * floord(floord(m + 1, 8), 4); c0 <= n; c0 += 1)
    s0(c0);

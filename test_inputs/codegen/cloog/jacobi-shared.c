if (2 * floord(h0 - 1, 2) + 1 == h0 && t1 >= 32 * floord(-g2 + t1 - 1, 32) + 3 && N + 32 * floord(-g2 + t1 - 1, 32) >= t1 + 1)
  for (int c0 = max(t0 - 16 * floord(t0 - 1, 16), t0 - 16 * floord(g1 + t0 - 3, 16)); c0 <= min(32, N - g1 - 1); c0 += 16)
    S1(g1 + c0 - 1, g2 + t1 + 32 * floord(-t1, 32) + 31);

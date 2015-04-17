if (N >= g0 + t1 + 1 && t1 <= 7 && g4 % 4 == 0)
  for (int c0 = t0, c0_end = min(127, N - g1 - 1); c0 <= c0_end; c0 += 16)
    S1(g0 + t1, g1 + c0);

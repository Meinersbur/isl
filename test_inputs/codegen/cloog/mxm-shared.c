if (g4 == 0 && t1 <= 7 && N >= g0 + t1 + 1) {
  for (int c0 = t0; c0 <= min(N - g1 - 1, 127); c0 += 16)
    S1(g0 + t1, g1 + c0);
} else if (g4 % 4 == 0 && N >= g0 + t1 + 1 && t1 <= 7 && g4 >= 4)
  for (int c0 = t0; c0 <= min(N - g1 - 1, 127); c0 += 16)
    S1(g0 + t1, g1 + c0);

for (int c0 = 0, c0_end = 5 * n; c0 <= c0_end; c0 += 1)
  for (int c1 = max(-((5 * n - c0 + 1) % 2) - n + c0 + 1, 2 * ((c0 + 2) / 3)), c1_end = min(c0, n + c0 - (n + c0 + 2) / 3); c1 <= c1_end; c1 += 2)
    S1((3 * c1 / 2) - c0, c0 - c1);

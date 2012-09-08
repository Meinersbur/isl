if (2 * floord(n, 2) == n)
  for (int c0 = (n + 4 * floord(-n - 1, 4) + 4) / 2; c0 <= 100; c0 += 2)
    S(c0);

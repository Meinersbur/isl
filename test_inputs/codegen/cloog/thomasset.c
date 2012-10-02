{
  for (int c0 = 0; c0 <= floord(n - 1, 3); c0 += 1)
    for (int c2 = 3 * c0 + 1; c2 <= min(n, 3 * c0 + 3); c2 += 1)
      S1(c2, c0);
  for (int c0 = floord(n, 3); c0 <= floord(n, 3) + floord(n, 3); c0 += 1)
    for (int c1 = 0; c1 < n; c1 += 1)
      for (int c3 = max(-n + 3 * c0, 1); c3 <= min(-n + 3 * c0 + 4, n); c3 += 1)
        if (((3 * c0 - c3 + 2) % 3) + n + c3 >= 3 * c0 + 2 && 3 * c0 + 4 >= ((3 * c0 - c3 + 2) % 3) + n + c3)
          S2(c1 + 1, c3, 0, n / 3, c0 - n / 3);
}

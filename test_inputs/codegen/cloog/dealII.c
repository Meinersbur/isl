{
  for (int c0 = 0; c0 <= min(T_2 - 1, T_66); c0 += 1) {
    S1(c0);
    S2(c0);
  }
  for (int c0 = max(T_66 + 1, 0); c0 < T_2; c0 += 1)
    S1(c0);
  if (T_67 == 0 && T_2 == 0)
    S1(0);
  for (int c0 = T_2; c0 <= min(T_66, T_67 - 1); c0 += 1)
    S2(c0);
}

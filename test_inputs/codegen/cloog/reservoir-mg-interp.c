{
  if (N >= 2)
    for (int c0 = 1, c0_end = O; c0 < c0_end; c0 += 1) {
      for (int c3 = 1, c3_end = M; c3 <= c3_end; c3 += 1)
        S1(c0, 1, c3);
      for (int c3 = 1, c3_end = M; c3 < c3_end; c3 += 1) {
        S6(c0, 1, c3);
        S7(c0, 1, c3);
      }
      if (N >= 3) {
        for (int c3 = 1, c3_end = M; c3 <= c3_end; c3 += 1)
          S3(c0, 1, c3);
        for (int c3 = 1, c3_end = M; c3 <= c3_end; c3 += 1)
          S1(c0, 2, c3);
        for (int c3 = 1, c3_end = M; c3 < c3_end; c3 += 1) {
          S6(c0, 2, c3);
          S7(c0, 2, c3);
        }
        for (int c3 = 1, c3_end = M; c3 < c3_end; c3 += 1)
          S11(c0, 1, c3);
      } else {
        for (int c3 = 1, c3_end = M; c3 <= c3_end; c3 += 1)
          S3(c0, 1, c3);
        for (int c3 = 1, c3_end = M; c3 < c3_end; c3 += 1)
          S11(c0, 1, c3);
      }
      for (int c1 = 3, c1_end = 2 * N - 4; c1 < c1_end; c1 += 2) {
        for (int c3 = 1, c3_end = M; c3 < c3_end; c3 += 1)
          S10(c0, (c1 - 1) / 2, c3);
        for (int c3 = 1, c3_end = M; c3 <= c3_end; c3 += 1)
          S3(c0, (c1 + 1) / 2, c3);
        for (int c3 = 1, c3_end = M; c3 <= c3_end; c3 += 1)
          S1(c0, (c1 + 3) / 2, c3);
        for (int c3 = 1, c3_end = M; c3 < c3_end; c3 += 1) {
          S6(c0, (c1 + 3) / 2, c3);
          S7(c0, (c1 + 3) / 2, c3);
        }
        for (int c3 = 1, c3_end = M; c3 < c3_end; c3 += 1)
          S11(c0, (c1 + 1) / 2, c3);
      }
      if (N >= 3) {
        for (int c3 = 1, c3_end = M; c3 < c3_end; c3 += 1)
          S10(c0, N - 2, c3);
        for (int c3 = 1, c3_end = M; c3 <= c3_end; c3 += 1)
          S3(c0, N - 1, c3);
        for (int c3 = 1, c3_end = M; c3 < c3_end; c3 += 1)
          S11(c0, N - 1, c3);
      }
      for (int c3 = 1, c3_end = M; c3 < c3_end; c3 += 1)
        S10(c0, N - 1, c3);
    }
  for (int c0 = 1, c0_end = O; c0 < c0_end; c0 += 1)
    for (int c1 = 1, c1_end = N; c1 < c1_end; c1 += 1) {
      for (int c3 = 1, c3_end = M; c3 <= c3_end; c3 += 1)
        S2(c0, c1, c3);
      for (int c3 = 1, c3_end = M; c3 < c3_end; c3 += 1)
        S8(c0, c1, c3);
      for (int c3 = 1, c3_end = M; c3 < c3_end; c3 += 1)
        S9(c0, c1, c3);
    }
  for (int c0 = 1, c0_end = O; c0 < c0_end; c0 += 1)
    for (int c1 = 1, c1_end = N; c1 < c1_end; c1 += 1)
      for (int c2 = 1, c2_end = M; c2 < c2_end; c2 += 1)
        S4(c0, c1, c2);
  for (int c0 = 1, c0_end = O; c0 < c0_end; c0 += 1)
    for (int c1 = 1, c1_end = N; c1 < c1_end; c1 += 1)
      for (int c2 = 1, c2_end = M; c2 < c2_end; c2 += 1)
        S5(c0, c1, c2);
  for (int c0 = R, c0_end = O; c0 < c0_end; c0 += 1)
    for (int c1 = Q, c1_end = N; c1 < c1_end; c1 += 1)
      for (int c2 = P, c2_end = M; c2 < c2_end; c2 += 1)
        S12(c0, c1, c2);
  for (int c0 = R, c0_end = O; c0 < c0_end; c0 += 1)
    for (int c1 = Q, c1_end = N; c1 < c1_end; c1 += 1)
      for (int c2 = 1, c2_end = M; c2 < c2_end; c2 += 1)
        S13(c0, c1, c2);
  for (int c0 = R, c0_end = O; c0 < c0_end; c0 += 1)
    for (int c1 = 1, c1_end = N; c1 < c1_end; c1 += 1)
      for (int c2 = P, c2_end = M; c2 < c2_end; c2 += 1)
        S14(c0, c1, c2);
  for (int c0 = R, c0_end = O; c0 < c0_end; c0 += 1)
    for (int c1 = 1, c1_end = N; c1 < c1_end; c1 += 1)
      for (int c2 = 1, c2_end = M; c2 < c2_end; c2 += 1)
        S15(c0, c1, c2);
}

{
  for (int c0 = 0, c0_end = 36; c0 <= c0_end; c0 += 1) {
    for (int c1 = -6, c1_end = c0 / 14 - 5; c1 < c1_end; c1 += 1) {
      for (int c2 = -((-2 * c1 + 3) / 5) + 9, c2_end = c1 + 12; c2 <= c2_end; c2 += 1)
        S6(c0, c1, c2);
      for (int c2 = c1 + 24, c2_end = -2 * c1 + 24; c2 <= c2_end; c2 += 1)
        S2(c0, c1, c2);
      for (int c2 = -2 * c1 + 30, c2_end = c1 + 48; c2 <= c2_end; c2 += 1)
        S1(c0, c1, c2);
    }
    for (int c1 = c0 / 14 - 5, c1_end = 0; c1 < c1_end; c1 += 1) {
      if (c1 >= -3 && 2 * c0 >= 7 * c1 + 42)
        S7(c0, c1, 6);
      for (int c2 = max(c1 - (6 * c0 + 77) / 77 + 13, -((-2 * c1 + 3) / 5) + 9), c2_end = c1 + 12; c2 <= c2_end; c2 += 1)
        S6(c0, c1, c2);
      for (int c2 = c1 - (3 * c0 + 14) / 14 + 49, c2_end = c1 + 48; c2 <= c2_end; c2 += 1)
        S1(c0, c1, c2);
    }
    S3(c0, 0, 0);
    S10(c0, 0, 0);
    for (int c2 = 1, c2_end = 5; c2 <= c2_end; c2 += 1)
      S3(c0, 0, c2);
    for (int c2 = 6, c2_end = 2 * c0 / 21 + 4; c2 <= c2_end; c2 += 1) {
      S3(c0, 0, c2);
      S7(c0, 0, c2);
    }
    for (int c2 = max(6, 2 * c0 / 21 + 5), c2_end = -((6 * c0 + 77) / 77) + 12; c2 <= c2_end; c2 += 1)
      S3(c0, 0, c2);
    for (int c2 = -((6 * c0 + 77) / 77) + 13, c2_end = 12; c2 <= c2_end; c2 += 1) {
      S3(c0, 0, c2);
      S6(c0, 0, c2);
    }
    for (int c2 = 13, c2_end = 24; c2 <= c2_end; c2 += 1)
      S3(c0, 0, c2);
    for (int c2 = -((3 * c0 + 14) / 14) + 49, c2_end = 48; c2 <= c2_end; c2 += 1)
      S1(c0, 0, c2);
    for (int c1 = 1, c1_end = 18; c1 <= c1_end; c1 += 1) {
      for (int c2 = -8 * c1, c2_end = min(6, -8 * c1 + 24); c2 <= c2_end; c2 += 1)
        S3(c0, c1, c2);
      if (c0 <= 34 && c1 == 1) {
        S3(c0, 1, 7);
      } else if (c1 == 2) {
        S3(c0, 2, 7);
      } else if (c0 >= 35 && c1 == 1) {
        S3(c0, 1, 7);
        S7(c0, 1, 7);
      }
      for (int c2 = 8, c2_end = min(-8 * c1 + 24, c1 - (6 * c0 + 77) / 77 + 12); c2 <= c2_end; c2 += 1)
        S3(c0, c1, c2);
      if (c1 == 1) {
        for (int c2 = -((6 * c0 + 77) / 77) + 14, c2_end = 13; c2 <= c2_end; c2 += 1) {
          S3(c0, 1, c2);
          S6(c0, 1, c2);
        }
        for (int c2 = 14, c2_end = 16; c2 <= c2_end; c2 += 1)
          S3(c0, 1, c2);
      }
      for (int c2 = max(-8 * c1 + 25, c1 - (6 * c0 + 77) / 77 + 13), c2_end = c1 + 12; c2 <= c2_end; c2 += 1)
        S6(c0, c1, c2);
      for (int c2 = c1 - (3 * c0 + 14) / 14 + 49, c2_end = c1 + 48; c2 <= c2_end; c2 += 1)
        S1(c0, c1, c2);
    }
    for (int c1 = 19, c1_end = 24; c1 <= c1_end; c1 += 1) {
      for (int c2 = -8 * c1, c2_end = -8 * c1 + 24; c2 <= c2_end; c2 += 1)
        S3(c0, c1, c2);
      for (int c2 = c1 - (6 * c0 + 77) / 77 + 13, c2_end = 30; c2 <= c2_end; c2 += 1)
        S6(c0, c1, c2);
    }
  }
  for (int c0 = 37, c0_end = 218; c0 <= c0_end; c0 += 1) {
    for (int c1 = (c0 + 5) / 14 - 8, c1_end = min(0, c0 / 14 - 5); c1 < c1_end; c1 += 1) {
      if (c0 <= 46 && c1 == -3)
        S7(c0, -3, 6);
      if (-77 * ((-3 * c1 + 1) / 5) + 447 >= 6 * c0)
        S6(c0, c1, -((-2 * c1 + 3) / 5) + 9);
      for (int c2 = c1 + 24, c2_end = -2 * c1 + 24; c2 <= c2_end; c2 += 1)
        S2(c0, c1, c2);
      for (int c2 = -2 * c1 + 30, c2_end = c1 - (3 * c0 + 17) / 14 + 56; c2 <= c2_end; c2 += 1)
        S1(c0, c1, c2);
    }
    if (c0 <= 148)
      for (int c1 = max(0, (c0 + 5) / 14 - 8), c1_end = c0 / 14 - 5; c1 < c1_end; c1 += 1) {
        if (c1 == 0)
          S2(c0, 0, 24);
        for (int c2 = max(c1 + 24, -2 * c1 + 30), c2_end = c1 - (3 * c0 + 17) / 14 + 56; c2 <= c2_end; c2 += 1)
          S1(c0, c1, c2);
      }
    if (c0 >= 79 && c0 % 14 >= 9) {
      for (int c2 = max((c0 - 70) / 14 + 24, (c0 - 70) / 14 - (3 * c0 + 14) / 14 + 49), c2_end = (c0 - 70) / 14 - (3 * c0 + 17) / 14 + 56; c2 <= c2_end; c2 += 1)
        S1(c0, c0 / 14 - 5, c2);
    } else if (c0 <= 69 && c0 % 14 >= 9) {
      if (c0 <= 41)
        S7(c0, -3, 6);
      S6(c0, c0 / 14 - 5, 8);
      for (int c2 = -((-c0 + 83) / 14) - (3 * c0 + 14) / 14 + 49, c2_end = -((-c0 + 83) / 14) - (3 * c0 + 17) / 14 + 56; c2 <= c2_end; c2 += 1)
        S1(c0, c0 / 14 - 5, c2);
    }
    for (int c1 = (c0 + 5) / 14 - 5, c1_end = 0; c1 < c1_end; c1 += 1) {
      if (7 * c1 + 114 >= 2 * c0)
        S7(c0, c1, 6);
      for (int c2 = max(8, c1 - (6 * c0 + 77) / 77 + 13), c2_end = c1 - (6 * c0 + 91) / 77 + 15; c2 <= c2_end; c2 += 1)
        S6(c0, c1, c2);
      for (int c2 = c1 - (3 * c0 + 14) / 14 + 49, c2_end = c1 - (3 * c0 + 17) / 14 + 56; c2 <= c2_end; c2 += 1)
        S1(c0, c1, c2);
    }
    for (int c1 = max(0, (c0 + 5) / 14 - 5), c1_end = c0 / 14 - 2; c1 < c1_end; c1 += 1) {
      for (int c2 = max(c1, -2 * c1 + 6), c2_end = min(min(-2 * c1 + 24, c1 - (6 * c0 + 91) / 77 + 15), (2 * c0 - 7 * c1 - 10) / 21 + 1); c2 <= c2_end; c2 += 1)
        S9(c0, c1, c2);
      if (c1 >= 1 && c1 <= 5 && 14 * c1 + 46 >= c0)
        S9(c0, c1, c1 + 5);
      for (int c2 = max(c1 + 6, (2 * c0 - 7 * c1 - 10) / 21 + 2), c2_end = (2 * c1 + 1) / 5 + 7; c2 <= c2_end; c2 += 1) {
        S7(c0, c1, c2);
        S9(c0, c1, c2);
      }
      if (c1 <= 3 && 7 * c1 + 21 * ((2 * c1 + 41) / 5) >= 2 * c0 + 12)
        S9(c0, c1, (2 * c1 + 1) / 5 + 8);
      for (int c2 = (2 * c1 + 1) / 5 + 9, c2_end = c1 - (6 * c0 + 91) / 77 + 15; c2 <= c2_end; c2 += 1) {
        S6(c0, c1, c2);
        S9(c0, c1, c2);
      }
      for (int c2 = c1 - (6 * c0 + 91) / 77 + 16, c2_end = -2 * c1 + 24; c2 <= c2_end; c2 += 1)
        S9(c0, c1, c2);
      for (int c2 = max(c1, -2 * c1 + 30), c2_end = min(c1 + 24, c1 - (3 * c0 + 17) / 14 + 47); c2 <= c2_end; c2 += 1)
        S8(c0, c1, c2);
      for (int c2 = max(c1 + 24, c1 - (3 * c0 + 14) / 14 + 49), c2_end = c1 - (3 * c0 + 17) / 14 + 56; c2 <= c2_end; c2 += 1)
        S1(c0, c1, c2);
    }
    for (int c1 = c0 / 14 - 2, c1_end = 18; c1 <= c1_end; c1 += 1) {
      for (int c2 = max(6, (c0 + 5) / 14 + 1), c2_end = min(min(c1, c0 / 14 + 3), -c1 + c1 / 2 + 18); c2 <= c2_end; c2 += 1)
        S5(c0, c1, c2);
      for (int c2 = c1 + 6, c2_end = min((2 * c1 + 1) / 5 + 7, (2 * c0 - 7 * c1 + 63) / 21 + 1); c2 <= c2_end; c2 += 1)
        S7(c0, c1, c2);
      for (int c2 = max(max(c1 + 6, c1 - (6 * c0 + 77) / 77 + 13), (2 * c1 + 1) / 5 + 9), c2_end = c1 - (6 * c0 + 91) / 77 + 15; c2 <= c2_end; c2 += 1)
        S6(c0, c1, c2);
      for (int c2 = max(c1 + (3 * c0 + 3) / 14 - 40, -c1 + (c1 + 1) / 2 + 21), c2_end = min(c1, c1 + 3 * c0 / 14 - 33); c2 <= c2_end; c2 += 1)
        S4(c0, c1, c2);
      for (int c2 = max(c1, c1 - (3 * c0 + 14) / 14 + 40), c2_end = min(c1 + 24, c1 - (3 * c0 + 17) / 14 + 47); c2 <= c2_end; c2 += 1)
        S8(c0, c1, c2);
      for (int c2 = max(c1 + 24, c1 - (3 * c0 + 14) / 14 + 49), c2_end = c1 - (3 * c0 + 17) / 14 + 56; c2 <= c2_end; c2 += 1)
        S1(c0, c1, c2);
    }
    for (int c1 = 19, c1_end = 24; c1 <= c1_end; c1 += 1) {
      for (int c2 = max(c1 - 12, (c0 + 5) / 14 + 1), c2_end = min(c0 / 14 + 3, -c1 + c1 / 2 + 18); c2 <= c2_end; c2 += 1)
        S5(c0, c1, c2);
      for (int c2 = max(max(c1 - 12, c1 + (3 * c0 + 3) / 14 - 40), -c1 + (c1 + 1) / 2 + 21), c2_end = min(c1, c1 + 3 * c0 / 14 - 33); c2 <= c2_end; c2 += 1)
        S4(c0, c1, c2);
      for (int c2 = max(c1 + 6, c1 - (6 * c0 + 77) / 77 + 13), c2_end = min(30, c1 - (6 * c0 + 91) / 77 + 15); c2 <= c2_end; c2 += 1)
        S6(c0, c1, c2);
      for (int c2 = max(c1, c1 - (3 * c0 + 14) / 14 + 40), c2_end = min(c1 + 24, c1 - (3 * c0 + 17) / 14 + 47); c2 <= c2_end; c2 += 1)
        S8(c0, c1, c2);
    }
    for (int c1 = 25, c1_end = min(42, -((3 * c0 + 17) / 14) + 71); c1 <= c1_end; c1 += 1)
      for (int c2 = max(c1 - 12, c1 + (3 * c0 + 3) / 14 - 40), c2_end = min(min(30, c1), c1 + 3 * c0 / 14 - 33); c2 <= c2_end; c2 += 1)
        S4(c0, c1, c2);
  }
}

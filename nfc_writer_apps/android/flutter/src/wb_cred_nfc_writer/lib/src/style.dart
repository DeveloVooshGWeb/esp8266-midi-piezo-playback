import 'package:flutter/material.dart';

abstract class Styles {
  static const String thin = 'MontserratThin';
  static const String extraLight = 'MontserratExtraLight';
  static const String light = 'MontserratLight';
  static const String regular = 'Montserrat';
  static const String medium = 'MontserratMedium';
  static const String semiBold = 'MontserratSemiBold';
  static const String bold = 'MontserratBold';
  static const String extraBold = 'MontserratExtraBold';
  static const String black = 'MontserratBlack';

  static const TextStyle thinStyle = TextStyle(
    fontFamily: thin,
    color: textColor,
  );

  static const TextStyle extraLightStyle = TextStyle(
    fontFamily: extraLight,
    color: textColor,
  );

  static const TextStyle lightStyle = TextStyle(
    fontFamily: light,
    color: textColor,
  );

  static const TextStyle regularStyle = TextStyle(
    fontFamily: regular,
    color: textColor,
  );

  static const TextStyle semiBoldBlackStyle = TextStyle(
    fontFamily: semiBold,
    color: Colors.black,
  );

  static const TextStyle mediumStyle = TextStyle(
    fontFamily: medium,
    color: textColor,
  );

  static const TextStyle semiBoldStyle = TextStyle(
    fontFamily: semiBold,
    color: textColor,
  );

  static const TextStyle boldStyle = TextStyle(
    fontFamily: bold,
    color: textColor,
  );

  static const TextStyle extraBoldStyle = TextStyle(
    fontFamily: extraBold,
    color: textColor,
  );

  static const TextStyle blackStyle = TextStyle(
    fontFamily: black,
    color: textColor,
  );

  static const Color bgFrom = Color(0xFF191B1C);
  static const Color bgTo = Color(0xFF191D1F);
  static const Color transparent = Color(0x00FFFFFF);
  static const Color teal = Color(0xFF009688);
  static const Color tealTranslucent = Color(0x44009688);
  static const Color boxTeal = Color.fromARGB(192, 40, 150, 139);
  // static const Color tealShadow = Color.fromARGB(110, 0, 129, 114);
  static const Color tealShadow = Color(0xBB2A2E2F);
  static const Color dialogTeal = Color.fromARGB(100, 120, 209, 179);
  static const Color placeholder = Color(0x77FFFFFF);
  static const Color textColor = Color(0xFFFFFFFF);
  static const Color errorColor = Color(0xFFFF0000);

  static const double padding = 15.0;
  static const double insAvatarRadius = 75.0;
  static const double insIconRadius = 25.0;
}

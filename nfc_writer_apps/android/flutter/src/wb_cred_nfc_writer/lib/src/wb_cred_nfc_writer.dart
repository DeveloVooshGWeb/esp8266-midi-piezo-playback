import 'dart:developer' as developer;
import 'dart:convert' as convert;
import 'dart:typed_data';

import 'package:flutter/material.dart';
import 'package:nfc_manager/nfc_manager.dart';
import 'package:wb_cred_nfc_writer/src/style.dart';
import 'package:wb_cred_nfc_writer/src/cred.dart';
import 'package:wb_cred_nfc_writer/src/formatter.dart';
import 'package:flutter_svg/flutter_svg.dart';

class WBApp extends StatelessWidget {
  const WBApp({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      theme: ThemeData(
        colorScheme: ColorScheme.fromSwatch(
          primarySwatch: Colors.teal,
        ),
        textTheme: const TextTheme(
          bodySmall: Styles.regularStyle,
          bodyMedium: Styles.mediumStyle,
          bodyLarge: Styles.boldStyle,
          titleSmall: Styles.regularStyle,
          titleMedium: Styles.mediumStyle,
          titleLarge: Styles.boldStyle,
        ),
      ),
      home: const Home(),
    );
  }
}

class Home extends StatefulWidget {
  const Home({Key? key}) : super(key: key);

  @override
  State<Home> createState() => _HomeState();
}

class _HomeState extends State<Home> {
  static const String title = 'WB Cred NFC Writer';
  final ssidController = TextEditingController();
  final passController = TextEditingController();

  final GlobalKey<FormState> _formKey = GlobalKey<FormState>();

  final List<Widget> _painters = <Widget>[];

  @override
  void initState() {
    super.initState();

    _painters.add(SvgPicture.asset(
      'assets/images/icon.svg',
    ));
  }

  @override
  void dispose() {
    ssidController.dispose();
    passController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text(title)),
      body: Stack(
        children: [
          Container(
            decoration: const BoxDecoration(
              gradient: LinearGradient(
                begin: Alignment.topCenter,
                end: Alignment.bottomCenter,
                colors: [
                  Styles.bgFrom,
                  Styles.bgTo,
                ],
                stops: [0.0, 1],
              ),
            ),
          ),
          Form(
            key: _formKey,
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: <Widget>[
                Padding(
                  padding: const EdgeInsets.symmetric(vertical: 16.0),
                  child: Align(
                    alignment: Alignment.center,
                    child: CircleAvatar(
                      backgroundColor: Colors.transparent,
                      radius: Styles.insIconRadius,
                      child: ClipRRect(
                        borderRadius: const BorderRadius.all(
                          Radius.circular(Styles.padding),
                        ),
                        child: _painters.toList()[0],
                      ),
                    ),
                  ),
                ),
                Padding(
                  padding: const EdgeInsets.symmetric(vertical: 16.0),
                  child: TextFormField(
                    decoration: const InputDecoration(
                      icon: Icon(
                        Icons.wifi,
                        color: Styles.teal,
                      ),
                      hintText: 'WiFi SSID',
                      hintStyle: TextStyle(
                        fontFamily: Styles.semiBold,
                        color: Styles.placeholder,
                      ),
                      errorStyle: TextStyle(
                        fontFamily: Styles.light,
                        color: Styles.errorColor,
                      ),
                      enabledBorder: UnderlineInputBorder(
                        borderSide: BorderSide(
                          color: Styles.tealTranslucent,
                          width: 2.0,
                        ),
                      ),
                      focusedBorder: UnderlineInputBorder(
                        borderSide: BorderSide(
                          color: Styles.tealShadow,
                          width: 2.0,
                        ),
                      ),
                      border: UnderlineInputBorder(),
                    ),
                    validator: (String? value) {
                      if (value == null || value.isEmpty) {
                        return 'SSID Is Blank';
                      }
                      return null;
                    },
                    controller: ssidController,
                  ),
                ),
                Padding(
                  padding: const EdgeInsets.symmetric(vertical: 16.0),
                  child: TextFormField(
                    decoration: const InputDecoration(
                      icon: Icon(
                        Icons.wifi_tethering,
                        color: Styles.teal,
                      ),
                      hintText: 'WiFi Password',
                      hintStyle: TextStyle(
                        fontFamily: Styles.semiBold,
                        color: Styles.placeholder,
                      ),
                      errorStyle: TextStyle(
                        fontFamily: Styles.light,
                        color: Styles.errorColor,
                      ),
                      enabledBorder: UnderlineInputBorder(
                        borderSide: BorderSide(
                          color: Styles.tealTranslucent,
                          width: 2.0,
                        ),
                      ),
                      focusedBorder: UnderlineInputBorder(
                        borderSide: BorderSide(
                          color: Styles.tealShadow,
                          width: 2.0,
                        ),
                      ),
                      border: UnderlineInputBorder(),
                    ),
                    validator: (String? value) {
                      if (value == null || value.isEmpty) {
                        return 'Password Is Blank';
                      }
                      return null;
                    },
                    controller: passController,
                  ),
                ),
                Padding(
                  padding: const EdgeInsets.symmetric(vertical: 16.0),
                  child: Align(
                    alignment: Alignment.center,
                    child: ElevatedButton(
                      onPressed: () {
                        if (_formKey.currentState!.validate()) {
                          NfcManager.instance.isAvailable().then((value) {
                            if (value) {
                              if (ssidController.text.length > 32) {
                                ssidController.text =
                                    ssidController.text.substring(0, 32);
                              }
                              if (passController.text.length > 64) {
                                passController.text =
                                    passController.text.substring(0, 64);
                              }
                              final String ssid = ssidController.text;
                              final String pass = passController.text;
                              ssidController.text = '';
                              passController.text = '';
                              developer.log("aaa");
                              showDialog(
                                context: context,
                                builder: (BuildContext context) {
                                  return NFCDialogBox(
                                    ssid: ssid,
                                    pass: pass,
                                  );
                                },
                                barrierColor: Styles.dialogTeal,
                              ).then((value) {
                                NfcManager.instance.stopSession();

                                developer.log("lololol");

                                if (value) {
                                  ssidController.text = 'Write Failed!';
                                  passController.text = 'Write Failed!';
                                }
                              });
                            } else {
                              ssidController.text = 'NFC Unsupported';
                              passController.text = 'NFC Unsupported';
                            }
                          });
                        }
                      },
                      child: const Text('Write'),
                    ),
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}

class NFCDialogBox extends StatefulWidget {
  final String ssid;
  final String pass;

  const NFCDialogBox({
    Key? key,
    required this.ssid,
    required this.pass,
  }) : super(key: key);

  @override
  State<NFCDialogBox> createState() => _NFCDialogBoxState();
}

class _NFCDialogBoxState extends State<NFCDialogBox>
    with SingleTickerProviderStateMixin {
  static const double minAlpha = 0.2;
  static const double maxAlpha = 1.0;

  final List<Widget> _painters = <Widget>[];

  late AnimationController _controller;
  late Animation<double> _animation;

  @override
  void initState() {
    super.initState();

    _painters.add(SvgPicture.asset(
      'assets/images/insertNFC.svg',
    ));

    _controller = AnimationController(
      duration: const Duration(seconds: 3),
      vsync: this,
      value: maxAlpha,
    )..repeat(
        min: minAlpha,
        max: maxAlpha,
        reverse: true,
      );

    _animation = CurvedAnimation(
      parent: _controller,
      curve: Curves.easeOut,
    );
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    NfcManager.instance.startSession(
      onDiscovered: (NfcTag tag) async {
        var ndef = Ndef.from(tag);

        if (ndef == null || !ndef.isWritable) {
          developer.log(ndef.toString());
          Navigator.of(context).pop();
          return;
        }

        developer.log(tag.data.toString());

        developer.log("wha");

        developer.log(widget.ssid);

        Uint8List byteArray = Uint8List.fromList(
          Formatter.wifiFormat(widget.ssid, widget.pass).codeUnits,
        );
        developer.log("fi");

        developer.log(byteArray.toString());

        /*for (int i = 0; i < 234 - formattedStr.length; i++) {
          formattedStr += String.fromCharCode(0);
        }*/

        NdefRecord record = NdefRecord(
          typeNameFormat: NdefTypeNameFormat.nfcWellknown,
          type: convert.base64.decode(Cred.dataType),
          identifier: Uint8List.fromList([]),
          payload: byteArray,
        );

        NdefMessage message = NdefMessage([record]);

        bool hasErr = false;

        try {
          await ndef.write(message);
          developer.log('Successfully Written...');
        } catch (e) {
          developer.log(e.toString());
          hasErr = true;
        }

        Navigator.of(context).pop(hasErr);
      },
    );

    return Dialog(
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.circular(Styles.padding),
      ),
      elevation: 0,
      backgroundColor: Styles.transparent,
      child: contentBox(context),
    );
  }

  contentBox(context) {
    return Stack(
      children: <Widget>[
        Container(
          padding: const EdgeInsets.only(
            left: Styles.padding,
            top: Styles.padding,
            right: Styles.padding,
            bottom: Styles.padding + Styles.insAvatarRadius,
          ),
          margin: const EdgeInsets.only(
            top: Styles.padding,
          ),
          decoration: BoxDecoration(
            shape: BoxShape.rectangle,
            color: Styles.bgTo,
            borderRadius: BorderRadius.circular(Styles.padding),
            boxShadow: const [
              BoxShadow(
                color: Styles.tealShadow,
                offset: Offset(0, 10),
                blurRadius: 10,
              ),
            ],
          ),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: const <Widget>[
              Text(
                'Insert NFC Tag',
                style: TextStyle(
                  fontSize: 25,
                ),
                textAlign: TextAlign.center,
              ),
              SizedBox(
                height: Styles.padding + Styles.insAvatarRadius,
              ),
              Align(
                alignment: Alignment.bottomRight,
                child: Text(''),
              ),
            ],
          ),
        ),
        Positioned(
          left: Styles.padding,
          right: Styles.padding,
          top: Styles.padding + Styles.insAvatarRadius,
          child: FadeTransition(
            opacity: _animation,
            child: CircleAvatar(
              backgroundColor: Colors.transparent,
              radius: Styles.insAvatarRadius,
              child: ClipRRect(
                borderRadius: const BorderRadius.all(
                  Radius.circular(Styles.padding),
                ),
                child: _painters.toList()[0],
              ),
            ),
          ),
        ),
      ],
    );
  }
}

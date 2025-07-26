import 'dart:developer' as developer;
import 'dart:convert' as convert;
import 'dart:typed_data';
import 'package:wb_cred_nfc_writer/src/cred.dart';

import 'package:pointycastle/block/aes_fast.dart';
import 'package:pointycastle/block/modes/cbc.dart';
import 'package:pointycastle/padded_block_cipher/padded_block_cipher_impl.dart';

import 'package:pointycastle/pointycastle.dart';
import 'package:pointycastle/paddings/pkcs7.dart';

abstract class Formatter {
  static String int2td_str(int num) {
    String numStr = num.toString();
    if (numStr.length < 2) numStr = "0" + numStr;
    numStr = numStr.substring(0, 2);
    return numStr;
  }

  static String wifiFormat(String ssid, String pass) {
    String header = String.fromCharCodes(convert.base64.decode(Cred.header));
    String formatted =
        header + ',' + int2td_str(ssid.length) + ',' + ssid + ',' + pass;
    Uint8List byteArray = Uint8List.fromList(formatted.codeUnits);
    /*
    encrypt.Key key = encrypt.Key.fromBase64(Cred.key);
    encrypt.IV iv = encrypt.IV.fromBase64(Cred.iv);
    encrypt.Encrypter encrypter = encrypt.Encrypter(encrypt.AES(
      key,
      mode: encrypt.AESMode.cbc,
    ));
    encrypt.Encrypted encrypted = encrypter.encrypt(
      formatted,
      iv: iv,
    );
    return encrypted.base64;
    */

    Uint8List key = convert.base64.decode(Cred.key);
    Uint8List iv = convert.base64.decode(Cred.iv);

    // ignore: deprecated_member_use
    CBCBlockCipher cbcCipher = CBCBlockCipher(AESFastEngine());

    ParametersWithIV<KeyParameter> ivParams =
        ParametersWithIV<KeyParameter>(KeyParameter(key), iv);

    // ignore: prefer_void_to_null
    PaddedBlockCipherParameters<ParametersWithIV<KeyParameter>, Null>
        paddingParams =
        // ignore: prefer_void_to_null
        PaddedBlockCipherParameters<ParametersWithIV<KeyParameter>, Null>(
            ivParams, null);

    PaddedBlockCipherImpl paddedCipher =
        PaddedBlockCipherImpl(PKCS7Padding(), cbcCipher);
    paddedCipher.init(true, paddingParams);

    try {
      return convert.base64.encode(paddedCipher.process(byteArray));
    } catch (e) {
      developer.log(e.toString());
      return '';
    }
  }
}

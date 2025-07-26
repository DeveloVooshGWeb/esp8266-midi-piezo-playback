#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "Base64.h"
// #include "AESLib.h"

// AESLib aesLib;

// #define INPUT_BUFFER_LIMIT (192 + 1)
// char encryptedBuffer[2*INPUT_BUFFER_LIMIT] = {0};
// char decryptedBuffer[INPUT_BUFFER_LIMIT] = {0};

const uint16_t Base64AlphabetSize = 64;
const char PROGMEM _Base64AlphabetTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

uint8_t credType[4] = { 0 };

byte keyByteArray[BLOCK_SIZE] = { 0 };
byte ivByteArray[BLOCK_SIZE] = { 0 };

byte temporaryIv1[BLOCK_SIZE] = { 0 };
byte temporaryIv2[BLOCK_SIZE] = { 0 };

String key = "";
String iv = "";

void setupKeys() {
  char* keyCharArray = (char*)key.c_str();
  char* ivCharArray = (char*)iv.c_str();

  int keyLength = key.length();
  int ivLength = iv.length();

  int decodedKeyLength = Base64.decodedLength(keyCharArray, keyLength);
  int decodedIvLength = Base64.decodedLength(ivCharArray, ivLength);

  if (decodedKeyLength == BLOCK_SIZE && decodedIvLength == BLOCK_SIZE) {
    char decodedKey[BLOCK_SIZE];
    char decodedIv[BLOCK_SIZE];

    Base64.decode(decodedKey, keyCharArray, keyLength);
    Base64.decode(decodedIv, ivCharArray, ivLength);

    for (int i = 0; i < BLOCK_SIZE; i++) {
      keyByteArray[i] = (byte)decodedKey[i];
      ivByteArray[i] = (byte)decodedIv[i];
    }
  }

  key = "";
  iv = "";

  // aesLib.set_paddingmode((paddingMode)0);
  // aesLib.set_paddingmode(paddingMode::Null);
}

/*
String aesEncrypt(char* msg, uint16_t msgLen) {
  memcpy(ivByteArray, temporaryIv1, BLOCK_SIZE);
  int cipherLength = aesLib.get_cipher64_length(msgLen);
  char outputBuffer[cipherLength + 1];
  aesLib.encrypt64(msg, msgLen, outputBuffer, keyByteArray, BLOCK_SIZE, temporaryIv1);
  for (uint8_t i = 0; i < BLOCK_SIZE; i++) temporaryIv1[i] = 0;
  return String(outputBuffer);
}

String aesDecrypt(char* msg, uint16_t msgLen) {
  memcpy(ivByteArray, temporaryIv2, BLOCK_SIZE);
  char outputBuffer[msgLen];
  aesLib.decrypt64(msg, msgLen, outputBuffer, keyByteArray, BLOCK_SIZE, temporaryIv2);
  for (uint8_t i = 0; i < BLOCK_SIZE; i++) temporaryIv2[i] = 0;
  return String(outputBuffer);
}

String aesEncrypt(String inputStr) {
  uint16_t inputLen = (uint16_t)inputStr.length();

  char inputBuffer[inputLen + 1];
  for (int i = 0; i < inputLen; i++) inputBuffer[i] = inputStr[i];

  return aesEncrypt(inputBuffer, inputLen);
}

String aesDecrypt(String inputStr) {
  uint16_t inputLen = (uint16_t)inputStr.length();
  
  char inputBuffer[inputLen + 1];
  for (int i = 0; i < inputLen; i++) inputBuffer[i] = inputStr[i];

  // return aesDecrypt(strdup(inputStr.c_str()), inputStr.length());
  return aesDecrypt(inputBuffer, inputLen);
}
*/

String aesEncrypt(String plainData){
  int i;
  int len = plainData.length();
  int nBlocks = len / 16 + 1;
  uint8_t nPadding = nBlocks * 16 - len;
  uint8_t data[nBlocks * 16];
  memcpy(data, plainData.c_str(), len);
  for(i = len; i < nBlocks * 16; i++){
    data[i] = nPadding;
  }
  
  uint8_t key[16], iv[16];
  memcpy(key, keyByteArray, 16);
  memcpy(iv, ivByteArray, 16);

  br_aes_big_cbcenc_keys encCtx;

  br_aes_big_cbcenc_init(&encCtx, key, 16);
  br_aes_big_cbcenc_run(&encCtx, iv, data, nBlocks * 16);

  len = nBlocks * 16;
  char encodedData[Base64.encodedLength(len)];
  Base64.encode(encodedData, (char*)data, len);
  
  return String(encodedData);
}

String aesDecrypt(String encodedData){  
  int encodedLen = encodedData.length();
  char *encoded = const_cast<char*>(encodedData.c_str());
  int len = Base64.decodedLength(encoded, encodedLen);
  uint8_t data[len];
  Base64.decode((char*)data, encoded, encodedLen);
  
  uint8_t key[16], iv[16];
  memcpy(key, keyByteArray, 16);
  memcpy(iv, ivByteArray, 16);

  int nBlocks = len / 16;

  br_aes_big_cbcdec_keys decCtx;

  br_aes_big_cbcdec_init(&decCtx, key, 16);
  br_aes_big_cbcdec_run(&decCtx, iv, data, nBlocks * 16);

  uint8_t nPadding = data[nBlocks * 16 - 1];
  len = nBlocks * 16 - nPadding;
  char plainData[len + 1];
  memcpy(plainData, data, len);
  plainData[len] = '\0';
  
  return String(plainData);
}

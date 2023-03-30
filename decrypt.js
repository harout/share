const crypto = require('crypto-js');
const fs = require('fs');


function convertWordArrayToUint8Array(wordArray) {
    var arrayOfWords = wordArray.hasOwnProperty("words") ? wordArray.words : [];
    var length = wordArray.hasOwnProperty("sigBytes") ? wordArray.sigBytes : arrayOfWords.length * 4;
    console.log(length);
    var uInt8Array = new Uint8Array(length), index=0, word, i;
    for (i=0; i<length; i++) {
        word = arrayOfWords[i];
        uInt8Array[index++] = word >> 24;
        uInt8Array[index++] = (word >> 16) & 0xff;
        uInt8Array[index++] = (word >> 8) & 0xff;
        uInt8Array[index++] = word & 0xff;
    }
    return uInt8Array;
}

var data = fs.readFileSync('data/twitter-stats.encrypted', null)
const decrypted = crypto.AES.decrypt(
    data.toString(),
    "password here"
);
const formatted = convertWordArrayToUint8Array(decrypted);


fs.writeFile("data/twitter-stats.decrypted", formatted, (err) => {
  if (err)
    console.log(err);
  else {
    console.log("File written successfully\n");
  }
});

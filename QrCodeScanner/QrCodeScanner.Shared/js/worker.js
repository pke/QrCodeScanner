/// <reference group="Dedicated Worker" />
importScripts("/js/qrcode-decoder.js");

onmessage = function (event) {
    qrcode.imagedata = { data: event.data.imagedata };
    qrcode.width = event.data.width;
    qrcode.height = event.data.height;
    qrcode.decode();
    if (qrcode.result) {
        postMessage(qrcode.result);
    } else {
        postMessage("");
    }
}

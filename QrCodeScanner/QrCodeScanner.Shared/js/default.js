// For an introduction to the Blank template, see the following documentation:
// http://go.microsoft.com/fwlink/?LinkID=392286
(function () {
    "use strict";

    var app = WinJS.Application;
    var activation = Windows.ApplicationModel.Activation;

    WinJS.Namespace.define("App", {
        codes: new WinJS.Binding.List(),
        lastCode: null
    })

    app.onactivated = function (args) {
        if (args.detail.kind === activation.ActivationKind.launch) {
            if (args.detail.previousExecutionState !== activation.ApplicationExecutionState.terminated) {
                // TODO: This application has been newly launched. Initialize
                // your application here.
            } else {
                // TODO: This application was suspended and then terminated.
                // To create a smooth user experience, restore application state here so that it looks like the app never stopped running.
            }
            var capturePreview = document.querySelector("video");
            var capture;
            var focusPromise;

            var stopCapture = function () {
                capturePreview.pause();
                capturePreview.src = null;
                capture && capture.stopRecordAsync();
                focusPromise && focusPromise.cancel();
                capture = null;
            }
            document.querySelector("button").onclick = stopCapture;
            Windows.Devices.Enumeration.DeviceInformation.findAllAsync(Windows.Devices.Enumeration.DeviceClass.videoCapture)
            .then(function (devices) {
                var deviceId;
                devices.forEach(function (device) {
                    if (device.enclosureLocation.panel && device.enclosureLocation.panel == Windows.Devices.Enumeration.Panel.back) {
                        deviceId = device.id;
                    }
                });
                return deviceId;
            }).then(function (deviceId) {
                capture = new Windows.Media.Capture.MediaCapture();
                var captureSettings = new Windows.Media.Capture.MediaCaptureInitializationSettings();
                captureSettings.streamingCaptureMode = Windows.Media.Capture.StreamingCaptureMode.video;
                captureSettings.videoDeviceId = deviceId;
                captureSettings.photoCaptureSource = Windows.Media.Capture.PhotoCaptureSource.videoPreview;
                capture.initializeAsync(captureSettings).done(function () {
                    var controller = capture.videoDeviceController;

                    if (controller.flashControl.supported) {
                        controller.flashControl.enabled = false;
                    }

                    if (controller.focusControl && controller.focusControl.supported) {
                        var refocus = function () {
                            focusPromise = WinJS.Promise.timeout(1000).then(function () {
                                return controller.focusControl.focusAsync().done(function () {
                                    setImmediate(refocus);
                                });
                            }).then(null, function () { });
                        }

                        if (controller.focusControl.configure) {
                            var focusConfig = new Windows.Media.Devices.FocusSettings();
                            focusConfig.autoFocusRange = Windows.Media.Devices.AutoFocusRange.macro;

                            var supportContinuousFocus = controller.focusControl.supportedFocusModes.indexOf(Windows.Media.Devices.FocusMode.continuous).returnValue;
                            var supportAutoFocus = controller.focusControl.supportedFocusModes.indexOf(Windows.Media.Devices.FocusMode.auto).returnValue;

                            if (supportContinuousFocus) {
                                focusConfig.mode = Windows.Media.Devices.FocusMode.continuous;
                            } else if (supportAutoFocus) {
                                focusConfig.mode = Windows.Media.Devices.FocusMode.auto;
                                refocus();
                            } else {
                                refocus();
                            }

                            controller.focusControl.configure(focusConfig);
                        }
                    }

                    var deviceProps = controller.getAvailableMediaStreamProperties(Windows.Media.Capture.MediaStreamType.videoRecord);

                    deviceProps = Array.prototype.slice.call(deviceProps);
                    deviceProps = deviceProps.filter(function (prop) {
                        // filter out streams with "unknown" subtype - causes errors on some devices
                        return prop.subtype !== "Unknown";
                    }).sort(function (propA, propB) {
                        // sort properties by resolution
                        return propB.width - propA.width;
                    });

                    var maxResProps = deviceProps[0];

                    controller.setMediaStreamPropertiesAsync(Windows.Media.Capture.MediaStreamType.videoRecord, maxResProps).done(function () {
                        // handle portrait orientation
                        if (Windows.Graphics.Display.DisplayProperties.nativeOrientation == Windows.Graphics.Display.DisplayOrientations.portrait) {
                            capture.setPreviewRotation(Windows.Media.Capture.VideoRotation.clockwise90Degrees);
                            capturePreview.msZoom = true;
                        }

                        var oncode, lastCode;
                        org.zxing.MultiFormatReader.addEventListener("oncode", oncode = function (event) {
                            //org.zxing.MultiFormatReader.removeEventListener("oncode", oncode);
                            var code = event.target;
                            Windows.Phone.Devices.Notification.VibrationDevice.getDefault().vibrate(4);
                            if (code != App.lastCode) {
                                App.codes.push({ code: code });
                                App.lastCode = code;
                            }
                            //stopCapture();
                        });
                        var definition = org.zxing.MultiFormatReaderDefinition(org.zxing.BarcodeFormat.qrCode || org.zxing.BarcodeFormat.dataMatrix, function (code) {
                            Windows.Phone.Devices.Notification.VibrationDevice.getDefault().vibrate(4);
                            if (code != App.lastCode) {
                                App.codes.push({ code: code });
                                App.lastCode = code;
                            }
                        });

                        capture.addEffectAsync(Windows.Media.Capture.MediaStreamType.videoPreview, definition.activatableClassId, definition.properties);
                        capturePreview.src = URL.createObjectURL(capture);
                        capturePreview.play();
                    });
                });
            });
            args.setPromise(WinJS.UI.processAll());
        }
    };

    app.oncheckpoint = function (args) {
        // TODO: This application is about to be suspended. Save any state
        // that needs to persist across suspensions here. You might use the
        // WinJS.Application.sessionState object, which is automatically
        // saved and restored across suspension. If you need to complete an
        // asynchronous operation before your application is suspended, call
        // args.setPromise().
    };

    app.start();
})();